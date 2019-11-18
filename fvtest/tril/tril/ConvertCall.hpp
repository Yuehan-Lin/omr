#ifndef CONVERTCALL_HPP
#define CONVERTCALL_HPP


#include "converter.hpp"

namespace Tril {

class ConvertCall: public ASTToTRNode {
    public:

    explicit ConvertCall(ASTToTRNode* next = NULL) : ASTToTRNode(next) {}

    protected:
        TR::Node* impl(const ASTNode* tree, TRNodeMaps* maps) override;
};

}
#endif