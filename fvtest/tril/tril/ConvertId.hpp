#ifndef CONVERTID_HPP
#define CONVERTID_HPP

#include "converter.hpp"


#define TraceEnabled    (comp()->getOption(TR_TraceILGen))
#define TraceIL(m, ...) {if (TraceEnabled) {traceMsg(comp(), m, ##__VA_ARGS__);}}


namespace Tril {

class ASTToTRNode;

class ConvertId: public ASTToTRNode {
    public:
    
    explicit ConvertId(ASTToTRNode* next = NULL) : ASTToTRNode(next) {}

    protected:
        TR::Node* impl(const ASTNode* tree, TRNodeMaps* maps) override;
};

}
#endif