#include "ConvertCall.hpp"
#include "ilgen.hpp"

#define TraceEnabled    (comp()->getOption(TR_TraceILGen))
#define TraceIL(m, ...) {if (TraceEnabled) {traceMsg(comp(), m, ##__VA_ARGS__);}}

namespace Tril {

TR::Node* ConvertCall::impl(const ASTNode* tree, TRNodeMaps* maps) {
            TR::Node* node = NULL;
            auto childCount = tree->getChildCount();
            auto opcode = OpCodeTable(tree->getName());
            if (opcode.isCall()) {
                auto compilation = TR::comp();

                const auto addressArg = tree->getArgByName("address");
                if (addressArg == NULL) {
                    //TraceIL("  Found call without required address associated\n");
                    return NULL;
                }

                /* I don't want to extend the ASTValue type system to include pointers at this moment,
                * so for now, we do the reinterpret_cast to pointer type from long
                */
#if defined(OMR_ENV_DATA64)
            //TraceIL("  is call with target 0x%016llX", addressArg->getValue()->get<uintptr_t>());
#else
            TraceIL("  is call with target 0x%08lX", addressArg->getValue()->get<uintptr_t>());
#endif /* OMR_ENV_DATA64 */
            const auto targetAddress = reinterpret_cast<void*>(addressArg->getValue()->get<uintptr_t>());

            /* To generate a call, will create a ResolvedMethodSymbol, but we need to know the
            * signature. The return type is intuitable from the call node, but the arguments
            * must be provided explicitly, hence the args list, that mimics the args list of
            * (method ...)
            */
            const auto argList = parseArgTypes(tree);

            auto argIlTypes = std::vector<TR::IlType*>(argList.size());
            auto output = argIlTypes.begin();
            for (auto iter = argList.begin(); iter != argList.end(); iter++, output++) {
                *output = maps->getTypes()->PrimitiveType(*iter);
            }

            auto returnIlType = maps->getTypes()->PrimitiveType(opcode.getType());

            TR::ResolvedMethod* method = new (compilation->trHeapMemory()) TR::ResolvedMethod("file",
                                                                                            "line",
                                                                                            "name",
                                                                                            argIlTypes.size(),
                                                                                            &argIlTypes[0],
                                                                                            returnIlType,
                                                                                            targetAddress,
                                                                                            0);
            //TODO
            TR::SymbolReference *methodSymRef = maps->symRefTab()->findOrCreateStaticMethodSymbol(JITTED_METHOD_INDEX, -1, method);

            /* Default linkage is always system, unless overridden */
            TR_LinkageConventions linkageConvention = TR_System;

            /* Calls can have a customized linkage */
            const auto* linkageArg= tree->getArgByName("linkage");
            if (linkageArg != NULL) {
                const auto* linkageString = linkageArg->getValue()->getString();
                linkageConvention = convertStringToLinkage(linkageString);
                if (linkageConvention == TR_None) {
                    //TraceIL("  failed to find customized linkage %s, aborting parsing\n", linkageString);
                    return NULL;
                }
                //TraceIL("  customizing linakge of call to %s (linkageConvention=%d)\n", linkageString, linkageConvention);
            }

            /* Set linkage explicitly */
            methodSymRef->getSymbol()->castToMethodSymbol()->setLinkage(linkageConvention);
            node  = TR::Node::createWithSymRef(opcode.getOpCodeValue(), childCount, methodSymRef);
     }
     else {
        //TraceIL("  unrecognized opcode; using default creation mechanism\n", "");
        node = TR::Node::create(opcode.getOpCodeValue(), childCount);
     }
     node->setFlags(parseFlags(tree));

     //TraceIL("  node address %p\n", node);
     //TraceIL("  node index n%dn\n", node->getGlobalIndex());

     auto nodeIdArg = tree->getArgByName("id");
     if (nodeIdArg != NULL) {
         auto id = nodeIdArg->getValue()->getString();
         maps->setNodemap(id, node);
         //TraceIL("  node ID %s\n", id);
     }

     return node;
}

}