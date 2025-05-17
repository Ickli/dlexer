#include <dlexer/regex.hpp>
#include <dlexer/common.hpp>
#include <cctype>
#include <cstring>
#include <sstream>
#include <cassert>

namespace dlexer {

namespace dtl {

template<typename T>
struct IsNodeVisitor: INodeVisitor {
    bool isT = false;

    void visit(T& _) override { isT = true; }
};

struct NameVisitor: INodeVisitor {
    const char* name;

    void visit(UnitNode& _) override { name = "UnitNode"; }
    void visit(StartNode& _) override { name = "StartNode"; }
    void visit(GroupNode& _) override { name = "GroupNode"; }
    void visit(OrNode& _) override { name = "OrNode"; }
    void visit(StarNode& _) override { name = "StarNode"; }
    void visit(EndNode& _) override { name = "EndNode"; }

    static const char* get(Node& n) {
        NameVisitor v;
        n.acceptVisitor(v);
        return v.name;
    }
};

template<typename T>
bool isNode(Node& n) {
    IsNodeVisitor<T> v{};
    n.acceptVisitor(v);
    return v.isT;
}

static void insertBetween(const Children_t& stack, Node& node, int at) {
    assert(at > 0 && at < stack.size() && "can't insert at the end or start");

    Node& parent = *stack[at-1];
    Node& child = *stack[at];
    
    assert(parent.children.back() == &child && "last child of parent must be on stack history");

    parent.children.back() = &node;
    node.children.push_back(&child);
}

} // namespace dtl

using namespace dtl;

/*
    1b
    \w*b
    \w+
    abc
    a|b
    ab|ba
    ^(abc)
*/
void RegexLexer::reprogram(const std::string& pat) {
    nodes.resize(1);
    err = nullptr;
    data = RegexData{};

    parsePattern(pat);
}

static void print(Node* n, int d, std::vector<Node*> traversed) {
    for(int j = 0; j < d; ++j) { std::cerr << '\t'; }

    for(Node* tr: traversed) {
        if(tr == n) { return; }
    }

    std::cerr << d << ' ' << NameVisitor::get(*n) << '\n';
    traversed.push_back(n);
    for(const auto child: n->children) {
        print(child, d+1, traversed);
    }
    traversed.pop_back();
}

int findLastSuperiorTo(const dtl::Node& node, const dtl::Children_t& stack);

// TODO: 1. use child adapters
//       2. if $\n pattern is met, reverse it to form the path: ('\n' -> $),
//       so we can set end of line at the '\n' symbol 
//       3. ^ returns true on BOTH start and end of line ( hehehehehe )
//          NOTE: if reverting, need to keep track of pos so if it's 0, put LINE_AT_START!
//       4. add seekPres (used by being compared to standPres of added ones)
//          add standPres (used by being compared to seekPres of being added one)
void RegexLexer::parsePattern(const std::string& pat) {
    std::vector<GroupNode*> groupStartStack;
    Children_t stack { nodes[0].get() };
    int ulen = 0;
    char unit[4];
    int unmatchedGroupId = -1;
    
    for(int byteInd = 0; byteInd < pat.size(); byteInd += ulen) {
        ulen = extractUnitStr(unit, pat.c_str() + byteInd);

        Node* newNode = nullptr;
        if(ulen > 1) { newNode = createNode<UnitNode>(unit, ulen); }
        else switch(unit[0]) {
        case '|': newNode = createNode<OrNode>(); break;
        case '(': {
            const int gid = groups.size();
            groups.push_back({});
            newNode = createNode<GroupNode>(gid, nullptr);
            // TODO: deal with (a|b)
            // newNode->presedence = OrNode::Presedence;
            groupStartStack.push_back(static_cast<GroupNode*>(newNode));
        } break;
        case ')': {
            GroupNode* start = groupStartStack.back();
            // start->presedence = GroupNode::Presedence;

            groupStartStack.pop_back();
            newNode = createNode<GroupNode>(start->groupId, start);
            start->paired = static_cast<GroupNode*>(newNode);
        } break;
        case '*': newNode = createNode<StarNode>(); break;
        default: newNode = createNode<UnitNode>(unit, ulen); break;
        } // else switch

        const int supAdapterAt = findLastSuperiorTo(*newNode, stack);
        Node& supAdapter = *stack[supAdapterAt];
        Node& curAdapter = *stack.back();

        if(&supAdapter == &curAdapter) {
            curAdapter.adaptChild(stack, *newNode, stack.size());
        } else {
            Node* endNode = createNode<EndNode>();
            {
                const size_t oldStackSize = stack.size();
                curAdapter.adaptChild(stack, *endNode, stack.size());
                assert(oldStackSize + 1 == stack.size() && "end node must only append to stack");
            }
            supAdapter.adaptChild(stack, *newNode, supAdapterAt + 1);
        }
    }

    stack.back()->adaptChild(stack, *createNode<EndNode>(), stack.size());

    std::cerr << "ENDENDNENDNENDN\n";
    std::vector<Node*> tr;
    print(nodes[0].get(), 0, tr);
    std::cerr << "ENDNDNDND\n";
}

std::string* RegexLexer::encodeAlpha(Children_t& stack, const char* unit, int ulen) {
    dtl::Node& parent = *stack.back();
    Node* newNode = createNode<UnitNode>(unit, ulen);

    parent.children.push_back(newNode);
    stack.push_back(newNode);
    return nullptr;
}

std::string* RegexLexer::encodeNonAlpha(Children_t& stack, char unit) {
    switch(unit) {
    case '(': return encodeStartCaptureGroup(stack);
    case ')': return encodeEndCaptureGroup(stack);
    case '|': return encodeOr(stack);
    default: return nullptr;
    } // switch
}

std::string* RegexLexer::encodeOr(Children_t& stack) {
    int i = stack.size() - 1;
    while(stack[i]->presedence < OrNode::Presedence && i >= 0) {
        --i;
    }
    if(i < 0) {
        std::cerr << "Tried push 'or' node, no nodes with higher presedence";
        std::exit(1);
    }

    stack.resize(i + 1); // resize up to parent

    if(isNode<OrNode>(*stack[i])) {
        Node* parent = stack[i];
        Node* oldChild = parent->children.back();
        Node* orNode = createNode<OrNode>();

        parent->children.back() = orNode;
        orNode->children.push_back(oldChild);

        stack.push_back(orNode);
    }

    return nullptr;
}

std::string* RegexLexer::encodeStartCaptureGroup(Children_t& stack) {
    // TODO
    return nullptr;
}

std::string* RegexLexer::encodeEndCaptureGroup(Children_t& stack) {
    // TODO
    return nullptr;
}

RegexLexer::RegexLexer(const std::string& pat) {
    nodes.push_back(std::make_unique<StartNode>());
    parsePattern(pat);
}

// returns true if:
//      a node with free children is found 
//      or there's some string to parse yet
// otherwise, returns false
static bool popUntilFreeChildren(std::string& out, RegexData& data, bool hasLastUnitFetched) {
    // TODO: look at the head of file 
    if(hasLastUnitFetched) { data.returnUnit(); }

    while(data.stack.size() > 1) {
        const NodeMem back = data.stack.back();
        if(back.firstUnprocessedChild < back.node->children.size()) {
            return true;
        }
        if(back.node->usage != UnitUsage_t::NoNeedInUnit) {
            const int ulen = data.returnUnit();
            out.erase(out.length() - ulen, ulen);
        }
        data.stack.pop_back();
    }

    const NodeMem startNode = data.stack[0];
    if(startNode.firstUnprocessedChild < startNode.node->children.size()) {
        return true;
    }

    data.stack[0].firstUnprocessedChild = 0;
    // proceed by one unit if whole pattern was impossible
    return data.extractUnit();
}

void RegexLexer::extractStringFromIstream(std::istream& s) {
    istreamString.clear();
    std::stringstream sstr;
    sstr << s.rdbuf();
    istreamString = sstr.str();
    istream = &s;
}

bool RegexLexer::getToken(std::string& out, std::istream& in) {
    if(istream != &in) { extractStringFromIstream(in); }
    return getToken(out, istreamString);
}

bool RegexLexer::getToken(std::string& out, const std::string& in) {
    data.str = &in;
    return getToken(out, data);
}

bool RegexLexer::getToken(std::string& out, RegexData& data) const {
    if(data.at == RegexData::LINE_AT_PAST_EOF) { return false; }
    // if(!data.reuseUnit && data.pos >= data.str->length()) { return false; }
    // if(data.at == RegexData::LINE_AT_EOF) { return false; }

    out.clear();
    data.stack.clear();

    data.stack.push_back({nodes[0].get(), 0});

    while(true) {
        NodeMem& curParent = data.stack.back();
        Node* cur = curParent.node->children[curParent.firstUnprocessedChild];
        // std::cerr << "PARENT: " << NameVisitor::get(*curParent.node) << " CHILD: " << NameVisitor::get(*cur) << '\n';

        const bool needsUnit = cur->usage != UnitUsage_t::NoNeedInUnit;
        const bool popUnitIfUnsatisfied = needsUnit & !data.reuseUnit;
        // Fetch unit if needed
        if(needsUnit) {
            // if can't fetch
            if(data.at == RegexData::LINE_AT_EOF || !(data.reuseUnit || data.extractUnit())) { 
                curParent.firstUnprocessedChild += 1;
                // false because eof and we haven't fetched anything
                if(!popUntilFreeChildren(out, data, false)) {
                    data.at = RegexData::LINE_AT_PAST_EOF;
                    return false;
                }
                continue;
            }
            data.reuseUnit = cur->usage == UnitUsage_t::ShareWithChild;
        }
        
        const int next = cur->satisfies(data);

        // if not satisfied, revert
        if(next == -1) {
            curParent.firstUnprocessedChild += 1;
            if(!popUntilFreeChildren(out, data, popUnitIfUnsatisfied)) {
                return false;
            }
            data.reuseUnit = false;
            continue;
        }

        // if needs unit, append it
        if(cur->usage != UnitUsage_t::NoNeedInUnit) {
            out.append(data.unit, data.ulen);
        }

        // std::cerr << "CUR CHILD SIZE: " << cur->children.size() << " POS: " << data.pos << " LEN: " << data.str->length() << '\n';
        // if end, return true
        if(cur->children.size() == 0) {
            if(out.length() == 0) {
                if(data.at == RegexData::LINE_AT_EOF) {
                    data.at = RegexData::LINE_AT_PAST_EOF;
                }
                data.extractUnit();
                data.reuseUnit = false;
            }
            return true;
        }

        // account current child of current parent
        curParent.firstUnprocessedChild += 1;
        data.stack.push_back({cur, next});
    } // while true
    
    return false;
}

int findLastSuperiorTo(const dtl::Node& node, const dtl::Children_t& stack) {
    int i = stack.size() - 1;
    for(; i >= 0; --i) {
        if(stack[i]->presedence >= node.presedence) {
            break;
        }
    }
    return i;
}

/***********************************  NODES  *******************************/

void RegexData::updateAt() {
    if(pos == str->length()) { at = LINE_AT_EOF; } 
    else if(ulen == 1 && unit[0] == '\n') { at = LINE_AT_END; }
    else { at = LINE_AT_MID; }
}

int RegexData::returnUnit() {
    const int ulen = unitLengthLast(str->c_str() + pos - 1);
    pos -= ulen;

    updateAt();

    return ulen;
}

bool RegexData::extractUnit() {
    if(pos < str->length()) {
        ulen = extractUnitStr(unit, str->c_str() + pos);
        pos += ulen;

        updateAt();
    } else {
        ulen = 0;
    }
    return ulen != 0;
}


UnitNode::UnitNode(const char* unitPtr, int ulen): ulen(ulen) {
    std::memcpy(this->unit, unitPtr, sizeof(unit));
}

int UnitNode::satisfies(RegexData& data) const {
    if((data.ulen == this->ulen) & (std::memcmp(this->unit, data.unit, data.ulen) == 0)) {
        return 0;
    }
    return -1;
}

void UnitNode::adaptChild(Children_t &stack, Node &node, int at) {
    // assuming that unit node has lowest pres
    assert(at == stack.size() && "child of unit node must be only appended");

    if(isNode<StarNode>(node)) {
        insertBetween(stack, node, stack.size() - 1);
        this->children.push_back(&node);

        stack.back() = &node;
    } else {
        stack.push_back(&node);
        this->children.push_back(&node);
    }
}

OrNode::OrNode() {}

int OrNode::satisfies(RegexData& data) const {
#if 0
    for(int childInd = 0; childInd < children.size(); ++childInd) {
        const Node* child = children[childInd];
        if(child->satisfies(data)) {
            return childInd;
        }
    }
#endif
    return 0;
}

void OrNode::adaptChild(Children_t &stack, Node &node, int at) {
    stack.resize(at);
    stack.push_back(&node);
    this->children.push_back(&node);
}

GroupNode::GroupNode(int id, GroupNode* p): groupId(id), paired(p) {
}

int GroupNode::satisfies(RegexData& data) const { return 0; }

void GroupNode::adaptChild(Children_t &stack, Node &node, int at) {
    // assume that child nodes come at the end of stack
    assert(at == stack.size() && "child of group node must be only appended");

    if(isNode<StarNode>(node)) {
        int startAt = stack.size() - 2;
        assert(stack.size() >= 3 && "stack must contain start, group start and group end");

        for(;startAt >= 0; --startAt) {
            Node& cur = *stack[startAt];
            if(isNode<GroupNode>(cur) 
            && static_cast<GroupNode&>(cur).groupId == this->groupId) {
                break;
            }
        }

        // TODO: don't assume startAt > 0 and return error if < 0
        assert(startAt > 0 && "group start node must be on the stack");

        insertBetween(stack, node, startAt); 
        this->children.push_back(&node);
        stack.resize(startAt + 1);
        stack.back() = &node;
    } else {
        this->children.push_back(&node);
        stack.push_back(&node);
    }
}

int StartNode::satisfies(RegexData& data) const { return -1; }

void StartNode::adaptChild(Children_t &stack, Node &node, int at) {
    assert(at == 1 && "start node only adapts at stack pos == 1");
    stack.resize(at);
    stack.push_back(&node);

    if(this->children.size() == 0) {
        this->children.push_back(&node);
    } else {
        Node* oldChild = this->children.back();
        this->children.back() = &node;
        node.children.push_back(oldChild);
    }
}

StarNode::StarNode() {}

int StarNode::satisfies(RegexData& data) const { return 0; }

void StarNode::adaptChild(Children_t &stack, Node &node, int at) {
    Node& firstChild = *this->children.back();
    Node* subAdapter = &firstChild;

    if(isNode<GroupNode>(firstChild)) {
        subAdapter = static_cast<Node*>(
            static_cast<GroupNode&>(firstChild).paired);
    }
    
    this->children.push_back(&node);
    subAdapter->children.push_back(&node);
    stack.push_back(&node);
}

EndNode::EndNode() {}

int EndNode::satisfies(RegexData& data) const { return 0; }

void EndNode::adaptChild(Children_t &stack, Node &node, int at) {
    std::cerr << "ERROR: attempt to push child to end node\n";
    std::exit(1);
}

} // namespace dlexer
