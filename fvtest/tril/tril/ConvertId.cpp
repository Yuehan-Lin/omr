#include "ConvertId.hpp"
#include "ilgen.hpp"

namespace Tril {

TR::Node* ConvertId::impl(const ASTNode* tree, TRNodeMaps* maps) {
            TR::Node* node = NULL;

            auto childCount = tree->getChildCount();

            if (strcmp("@id", tree->getName()) == 0) {
                auto id = tree->getPositionalArg(0)->getValue()->getString();
                auto iter = maps->findNodeByID(id);
                if (iter != maps->nodeMapEnd()) {
                    auto n = iter->second;
                    //TraceIL("Commoning node n%dn (%p) from ASTNode %p (@id \"%s\")\n", n->getGlobalIndex(), n, tree, id);
                    return n;
                }
                else {
                    //TraceIL("Failed to find node for commoning (@id \"%s\")\n", id)
                    return NULL;
                }
            }
            else if (strcmp("@common", tree->getName()) == 0) {
                auto id = tree->getArgByName("id")->getValue()->getString();
                //TraceIL("WARNING: Using @common is deprecated. Please use (@id \"%s\") instead.\n", id);
                fprintf(stderr, "WARNING: Using @common is deprecated. Please use (@id \"%s\") instead.\n", id);
                auto iter = maps->findNodeByID(id);
                if (iter != maps->nodeMapEnd()) {
                    auto n = iter->second;
                    //TraceIL("Commoning node n%dn (%p) from ASTNode %p (@id \"%s\")\n", n->getGlobalIndex(), n, tree, id);
                    return n;
                }
                else {
                    //TraceIL("Failed to find node for commoning (@id \"%s\")\n", id)
                    return NULL;
                }
            }

            auto opcode = OpCodeTable(tree->getName());
            //TraceIL("Creating %s from ASTNode %p\n", opcode.getName(), tree);
            if (opcode.isLoadConst()) {
                //TraceIL("  is load const of ", "");
                node = TR::Node::create(opcode.getOpCodeValue(), childCount);
                auto value = tree->getPositionalArg(0)->getValue();

                // assume the constant to be loaded is the first argument of the AST node
                if (opcode.isIntegerOrAddress()) {
                    auto v = value->get<int64_t>();
                    node->set64bitIntegralValue(v);
                    //TraceIL("integral value %d\n", v);
                }
                else {
                    switch (opcode.getType()) {
                        case TR::Float:
                            node->setFloat(value->get<float>());
                            break;
                        case TR::Double:
                            node->setDouble(value->get<double>());
                            break;
                        default:
                            return NULL;
                    }
                    //TraceIL("floating point value %f\n", value->getFloatingPoint());
                }
            }
            else if (opcode.isLoadDirect()) {
                //TraceIL("  is direct load of ", "");

                // the name of the first argument tells us what kind of symref we're loading
                if (tree->getArgByName("parm") != NULL) {
                    auto arg = tree->getArgByName("parm")->getValue()->get<int32_t>();
                    //TraceIL("parameter %d\n", arg);
                    auto symref = maps->symRefTab()->findOrCreateAutoSymbol(maps->methodSymbol(), arg, opcode.getType() );
                    node = TR::Node::createLoad(symref);
                }
                else if (tree->getArgByName("temp") != NULL) {
                    const auto symName = tree->getArgByName("temp")->getValue()->getString();
                    //TraceIL("temporary %s\n", symName);
                    auto symref = maps->findSymRefByName(symName)->second;
                    node = TR::Node::createLoad(symref);
                }
                else {
                    // symref kind not recognized
                    return NULL;
                }
            }
            else if (opcode.isStoreDirect()) {
                //TraceIL("  is direct store of ", "");

                // the name of the first argument tells us what kind of symref we're storing to
                if (tree->getArgByName("temp") != NULL) {
                    const auto symName = tree->getArgByName("temp")->getValue()->getString();
                    //TraceIL("temporary %s\n", symName);

                    // check if a symref has already been created for the temp
                    // and if not, create one
                    if (maps->findSymRefByName(symName) == maps->symRefMapEnd()) {
                        maps->setSymRefPair(symName, maps->symRefTab()->createTemporary(maps->methodSymbol(), opcode.getDataType()));
                    }

                    auto symref = maps->findSymRefByName(symName)->second;
                    node = TR::Node::createWithSymRef(opcode.getOpCodeValue(), childCount, symref);
                }
                else {
                    // symref kind not recognized
                    return NULL;
                }
            }
            else if (opcode.isLoadIndirect() || opcode.isStoreIndirect()) {
                //TraceIL("  is indirect store/load with ");
                // If not specified, offset will default to zero.
                int32_t offset = 0;
                if (tree->getArgByName("offset")) {
                    offset = tree->getArgByName("offset")->getValue()->get<int32_t>();
                } else {
                    //TraceIL(" (default) ");
                }
                //TraceIL("offset %d ", offset);

                const auto name = tree->getName();
                auto compilation = TR::comp();
                TR::DataType type;
                if (opcode.isVector()) {
                    // Vector types in TR IL are "typeless", insofar as they are
                    // supposed to infer the vector type depending on the children.
                    // Loads determine their data type based on the symref. However,
                    // given that we are creating a symref here, we need a hint as to
                    // what type of symref to create. So, vloadi and vstorei will take
                    // an extra argument "type" to annotate the type desired.
                    if (tree->getArgByName("type") != NULL) {
                        auto nameoftype = tree->getArgByName("type")->getValue()->getString();
                        type = getTRDataTypes(nameoftype);
                        //TraceIL(" of vector type %s\n", nameoftype);
                    } else {
                        return NULL;
                    }
                } else {
                    //TraceIL("\n");
                    type = opcode.getType();
                }
                TR::Symbol* sym = TR::Symbol::createNamedShadow(compilation->trHeapMemory(), type, TR::DataType::getSize(opcode.getType()), (char*)name);
                TR::SymbolReference* symref = new (compilation->trHeapMemory()) TR::SymbolReference(compilation->getSymRefTab(), sym, compilation->getMethodSymbol()->getResolvedMethodIndex(), -1);
                symref->setOffset(offset);
                node = TR::Node::createWithSymRef(opcode.getOpCodeValue(), childCount, symref);
            }
            else if (opcode.isIf()) {
                const auto targetName = tree->getArgByName("target")->getValue()->getString();
                auto targetId = maps->findBlockByName(targetName);
                auto targetEntry = maps->blocks()[targetId]->getEntry();
                //TraceIL("  is if with target block %d (%s, entry = %p", targetId, targetName, targetEntry);

                /* If jumps must be created using `TR::Node::createif()`, which expected
                * two child nodes to be given as argument. However, because children
                * are only processed at the end, we create a dummy `BadILOp` node and
                * pass it as both the first and second child. When the children are
                * eventually created, they will override the dummy.
                */
                auto c1 = TR::Node::create(TR::BadILOp);
                auto c2 = c1;
                //TraceIL("  created temporary %s n%dn (%p)\n", c1->getOpCode().getName(), c1->getGlobalIndex(), c1);
                node = TR::Node::createif(opcode.getOpCodeValue(), c1, c2, targetEntry);
            }
            else if (opcode.isBranch()) {
                const auto targetName = tree->getArgByName("target")->getValue()->getString();
                auto targetId = maps->findBlockByName(targetName);
                auto targetEntry = maps->blocks()[targetId]->getEntry();
                //TraceIL("  is branch to target block %d (%s, entry = %p", targetId, targetName, targetEntry);
                node = TR::Node::create(opcode.getOpCodeValue(), childCount);
                node->setBranchDestination(targetEntry);
            }
            else {
                return NULL;
            }
            return node;
        }
}