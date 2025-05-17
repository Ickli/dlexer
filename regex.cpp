#include <dlexer/regex.hpp>
#include <dlexer/common.hpp>
#include <cctype>
#include <cstring>
#include <sstream>
#include <cassert>
#include <algorithm>

namespace dlexer {

namespace dtl {

struct IsSpecialVisitor: INodeVisitor {
    bool is = false;

    void visit(AtStartNode& _) override { is = true; }
    void visit(AtEndNode& _) override { is = true; }
    void visit(UnitNode& _) override { is = true; }

    static bool check(Node& n) {
        IsSpecialVisitor v;
        n.acceptVisitor(v);
        return v.is;
    }
};

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
    void visit(AtStartNode& _) override { name = "AtStartNode"; }
    void visit(AtEndNode& _) override { name = "AtEndNode"; }
    void visit(RangeNode& _) override { name = "RangeNode"; }

    static const char* get(Node& n) {
        NameVisitor v;
        n.acceptVisitor(v);
        return v.name;
    }
};

template<typename T>
T* isNode(Node& n) {
    IsNodeVisitor<T> v{};
    n.acceptVisitor(v);
    return v.isT ? &static_cast<T&>(n) : nullptr;
}

static void insertBetween(const Children_t& stack, Node& node, int at) {
    assert(at > 0 && at <= stack.size() && "can't insert past the end or at start");

    Node& parent = *stack[at-1];
    
    if(at < stack.size()) {
        parent.children.back() = &node;
        node.children.push_back(stack[at]);
    } else {
        parent.children.push_back(&node);
    }
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
    for(Node* tr: traversed) {
        if(tr == n) { return; }
    }

    for(int j = 0; j < d; ++j) { std::cerr << "        "; }

    std::cerr << d << ' ' << NameVisitor::get(*n) << '\n';
    traversed.push_back(n);
    for(const auto child: n->children) {
        print(child, d+1, traversed);
    }
    traversed.pop_back();
}

int findLastSuperiorTo(dtl::Node& node, const dtl::Children_t& stack);

void RegexLexer::parsePattern(const std::string& pat) {
    std::vector<GroupNode*> groupStartStack;
    Children_t stack { nodes[0].get() };
    int ulen = 0;
    char unit[4];
    int unmatchedGroupId = -1;
    bool isEscaped = false;
    
    for(int byteInd = 0; byteInd < pat.size(); byteInd += ulen) {
        ulen = extractUnitStr(unit, pat.c_str() + byteInd);

        Node* newNode = nullptr;
        if(ulen > 1) { newNode = createNode<UnitNode>(unit, ulen); }
        else if(isEscaped) { 
            newNode = createNode<UnitNode>(unit, ulen); 
            isEscaped = false;
        }
        else switch(unit[0]) {
        case '|': newNode = createNode<OrNode>(); break;
        case '(': {
            const int gid = groups.size();
            groups.push_back({});
            newNode = createNode<GroupNode>(gid, nullptr, false);
            groupStartStack.push_back(static_cast<GroupNode*>(newNode));
        } break;
        case ')': {
            GroupNode* start = groupStartStack.back();
            groupStartStack.pop_back();
            newNode = createNode<GroupNode>(start->groupId, start, true);
            start->paired = static_cast<GroupNode*>(newNode);
        } break;
        case '*': newNode = createNode<StarNode>(); break;
        case '^': newNode = createNode<AtStartNode>(); break;
        case '$': newNode = createNode<AtEndNode>(); break;
        case '\\': isEscaped = true; continue;
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

#if 0
    std::cerr << "START: " << pat << '\n';
    std::vector<Node*> tr;
    print(nodes[0].get(), 0, tr);
    std::cerr << "ENDNDNDND\n";
#endif
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

    out.clear();
    data.stack.clear();

    data.stack.push_back({nodes[0].get(), 0});

    while(true) {
        NodeMem& curParent = data.stack.back();
        Node* cur = curParent.node->children[curParent.firstUnprocessedChild];

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

int findLastSuperiorTo(dtl::Node& node, const dtl::Children_t& stack) {
    int i = stack.size() - 1;
    IsSpecialVisitor checker;
    node.acceptVisitor(checker);

    if(checker.is || !node.skipSpecials) { return i; }
    checker.is = false;

    for(; i >= 0; --i) {
        stack[i]->acceptVisitor(checker);
        if(!checker.is && stack[i]->pres >= node.pres) {
            break;
        }
        checker.is = false;
    }
    return i;
}

/***********************************  NODES  *******************************/

bool RegexData::isCurOrNextNewLine() const {
    if(ulen == 1 && unit[0] == '\n') { return true; }
    if(unitLength((*str)[pos]) == 1 && (*str)[pos] == '\n') { return true; }
    return false;
}

void RegexData::updateAt() {
    if(pos == str->length()) { at = LINE_AT_EOF; } 
    //else if(ulen == 1 && unit[0] == '\n') { at = LINE_AT_END; }
    else if(isCurOrNextNewLine()) { at = LINE_AT_END; }
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
    return 0;
}

void OrNode::adaptEndGroupNode(GroupNode& node, Node& curParent, std::vector<Node*>& visit) {
    const auto found = 
        std::find(visit.cbegin(), visit.cend(), &curParent) != visit.cend();
    if(found) { return; }

    assert(curParent.children.size() != 0 && "leaves must have end node");

    Node& back = *curParent.children.back();

    if(auto backEnd = isNode<EndNode>(back); backEnd) {
        assert(curParent.children.size() == 1);
        curParent.children.back() = &node;
        return;
    }

    visit.push_back(&curParent);
    for(auto child: curParent.children) {
        adaptEndGroupNode(node, *child, visit);
    }
}

void OrNode::adaptChild(Children_t &stack, Node &node, int at) {
    GroupNode* gnode = isNode<GroupNode>(node);
    if(gnode != nullptr && gnode->isEnd()) {
        std::vector<Node*> visit;
        adaptEndGroupNode(*gnode, *this, visit);
        stack.push_back(&node);
        return;
    }

    if(isNode<OrNode>(node) != nullptr) {
        stack.resize(at);
        return;
    }
    
    stack.resize(at);
    stack.push_back(&node);
    this->children.push_back(&node);
}

GroupNode::GroupNode(int id, GroupNode* p, bool isEnd): groupId(id), paired(p), NodeCRTP(isEnd) {}

bool GroupNode::isEnd() const { return skipSpecials; }
int GroupNode::satisfies(RegexData& data) const { return 0; }

void GroupNode::lowerPresedence() {
    assert(paired != nullptr);

    pres = paired->pres = GroupNode::PairedPresedence;
}

void GroupNode::adaptChild(Children_t &stack, Node &node, int at) {
    const GroupNode* gnode = isNode<GroupNode>(node);
    const bool isNodeEnd = gnode != nullptr && gnode->isEnd();

    if(isNode<StarNode>(node)) {
        int startAt = stack.size() - 2;
        assert(at == stack.size() && "star node must be appended at stack end");
            /*
            if(auto gcur = isNode<GroupNode>(cur);
            gcur && gcur->groupId == this->groupId) {
                break;
            }
            */
        assert(stack.size() >= 3 && "stack must contain start, group start and group end");

        for(;startAt >= 0; --startAt) {
            Node& cur = *stack[startAt];
            if(&cur == this->paired) { break; }
        }

        // TODO: don't assume startAt > 0 and return error if < 0
        assert(startAt > 0 && "group start node must be on the stack");

        insertBetween(stack, node, startAt); 
        this->children.push_back(&node);
        stack.resize(startAt + 1);
        stack.back() = &node;
    } else if(isNodeEnd && stack.back() != this) { 
        // if group end node, and EndNode at stack's top
        assert(stack.size() >= 3 && "start, group start and end nodes must be on stack");
        stack.back() = &node;
        stack[stack.size() - 2]->children.back() = &node;
        lowerPresedence();
    } else {
        insertBetween(stack, node, at);
        stack.resize(at);
        stack.push_back(&node);
        if(isNodeEnd) { lowerPresedence(); }
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

AtStartNode::AtStartNode() {}

int AtStartNode::satisfies(RegexData& data) const { 
    // matches both start and end (where end is line or file end)
    return -1 * !(data.at != RegexData::LINE_AT_MID);
}

void AtStartNode::adaptChild(Children_t &stack, Node &node, int at) {
    assert(at == stack.size() && "AtStartNode's child must be appended");

    stack.push_back(&node);
    this->children.push_back(&node);
}

AtEndNode::AtEndNode() {}

int AtEndNode::satisfies(RegexData& data) const { 
    return -1 * !(data.at == RegexData::LINE_AT_EOF 
        || data.at == RegexData::LINE_AT_END);
}

void AtEndNode::adaptChild(Children_t &stack, Node &node, int at) {
    assert(at == stack.size() && "AtEndNode's child must be appended");

    UnitNode* unit = isNode<UnitNode>(node);
    if(unit != nullptr && unit->ulen == 1 && unit->unit[0] == '\n') {
        insertBetween(stack, node, at - 1);
        stack.back() = &node;
        stack.push_back(this);
        return;
    }

    stack.push_back(&node);
    this->children.push_back(&node);
}

RangeNode::RangeNode(const char* start, const char* end) {}

int RangeNode::satisfies(RegexData& data) const {
    for(int i = 0; i < sizeof(start); ++i) {
        const unsigned char cur = data.unit[i];
        if(cur < start[i] || cur > end[i]) { return -1; }
    }
    return 0;
}

void RangeNode::adaptChild(Children_t &stack, Node &node, int at) {
    assert(at == stack.size() && "child of RangeNode must be only appended");

    if(isNode<StarNode>(node)) {
        insertBetween(stack, node, stack.size() - 1);
        this->children.push_back(&node);

        stack.back() = &node;
    } else {
        stack.push_back(&node);
        this->children.push_back(&node);
    }
}

} // namespace dlexer
