#ifndef ASTTOTRNODE_HPP
#define ASTTOTRNODE_HPP

#include "ast.hpp"
#include "ilgen/TypeDictionary.hpp"
#include "maps.hpp"


namespace Tril {

class ASTToTRNode {
    public:
        TR::Node* convert(const ASTNode *t, TRNodeMaps* map) {
            TRNodeMaps* maps = new TRNodeMaps();
            auto* n = impl(t, maps);
            if (n == NULL && next != NULL) {
                    return next->convert(t, map);
            } else {
                    return n;
            }
        }
        explicit ASTToTRNode (ASTToTRNode* n = NULL) : next(n) {}

    protected:
        virtual TR::Node* impl(const ASTNode*, TRNodeMaps* maps) = 0;
    
    private:
        ASTToTRNode* next;
};

}
#endif