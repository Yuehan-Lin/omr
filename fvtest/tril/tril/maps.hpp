#ifndef TRNODEMAPS_HPP
#define TRNODEMAPS_HPP

#include "ilgen/TypeDictionary.hpp"

class TRNodeMaps {
    TR::TypeDictionary* _types;
    std::map<std::string, TR::SymbolReference*> _symRefMap;
    std::map<std::string, int> _blockMap;
    std::map<std::string, TR::Node*> _nodeMap;
    
    protected:
        TR::SymbolReferenceTable     * _symRefTab;
        TR::ResolvedMethodSymbol     * _methodSymbol;
        TR::Block                   ** _blocks;

    public:
        std::map<std::string, TR::Node*>::iterator findNodeByID(const char * id) {
            return _nodeMap.find(id);
        }
        std::map<std::string, TR::Node*>::iterator nodeMapEnd() {
            return _nodeMap.end();
        }
        void setNodemap(const char *id, TR::Node *node) {
            _nodeMap[id] = node;
        }
        std::map<std::string, TR::SymbolReference*>::iterator findSymRefByName(const char *symName) {
            return _symRefMap.find(symName);
        }
        void setSymRefPair(const char *symName, TR::SymbolReference* symRef) {
            _symRefMap[symName] = symRef;
        }
        std::map<std::string, TR::SymbolReference*>::iterator symRefMapEnd() {
            return _symRefMap.end();
        }
        int findBlockByName(const char * targetName) {
            return _blockMap[targetName];
        }
        TR::TypeDictionary* getTypes() {
            return _types;
        }
        TR::SymbolReferenceTable     * symRefTab()              { return _symRefTab; }
        TR::ResolvedMethodSymbol     * methodSymbol() const     { return _methodSymbol; }
        TR::Block                   ** blocks() const           { return _blocks; }
};

#endif