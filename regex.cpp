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
    // void visit(GroupNode& g) override { is = g.isEnd(); }

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
    void visit(RepeatNode& _) override { name = "RepeatNode"; }
    void visit(EndNode& _) override { name = "EndNode"; }
    void visit(AtStartNode& _) override { name = "AtStartNode"; }
    void visit(AtEndNode& _) override { name = "AtEndNode"; }
    void visit(RangeNode& _) override { name = "RangeNode"; }
    void visit(FailNode& _) override { name = "FailNode"; }

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

void adaptEndGroupNode_(GroupNode& end, Node& curParent, Children_t& visit) {
    const auto found = 
        std::find(visit.cbegin(), visit.cend(), &curParent) != visit.cend();
    if(found) { return; }

    const size_t childrenSize = curParent.children.size();
    assert(curParent.children.size() > 0 
        && "leaves must have at least end node");

    visit.push_back(&curParent);
    for(int childInd = 0; childInd < curParent.children.size(); ++childInd) {
        Node* const child = curParent.children[childInd];
        if(isNode<EndNode>(*child)) { curParent.children[childInd] = &end; }
        else if(isNode<FailNode>(*child)) { continue; }
        else { adaptEndGroupNode_(end, *child, visit); }
    }
}

void adaptEndGroupNode(GroupNode& end, Node& curParent) {
    Children_t visit;
    adaptEndGroupNode_(end, curParent, visit);
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

void RegexLexer::appendNode(dtl::Children_t& stack, dtl::Node* newNode, bool addEnd) {
    const int supAdapterAt = findLastSuperiorTo(*newNode, stack);
    Node& supAdapter = *stack[supAdapterAt];
    Node& curAdapter = *stack.back();

    if(&supAdapter == &curAdapter) {
        curAdapter.adaptChild(stack, *newNode, stack.size());
    } else {
        Node* endNode = createNode<EndNode>();

        if(addEnd) {
            const size_t oldStackSize = stack.size();
            curAdapter.adaptChild(stack, *endNode, stack.size());
            assert(oldStackSize + 1 == stack.size() && "end node must only append to stack");
        }
        supAdapter.adaptChild(stack, *newNode, supAdapterAt + 1);
    }
}

void RegexLexer::appendOrGroupNode(dtl::Children_t& stack, std::vector<dtl::Node*> orGroup, bool isExclusive) {
    GroupNode* groupStart = static_cast<GroupNode*>(
        createNode<GroupNode>(-1, nullptr, false, false));
    GroupNode* groupEnd = static_cast<GroupNode*>(
        createNode<GroupNode>(-1, groupStart, true, false));
    groupStart->paired = groupEnd;

    Node* ornode = createNode<OrNode>(isExclusive);

    appendNode(stack, groupStart, true);
    appendNode(stack, ornode, true);

    static const int toPopCount = 2;
    for(Node* orchild: orGroup) {
        appendNode(stack, orchild, true);

        if(isExclusive) { appendNode(stack, createNode<FailNode>(), true);
        } else { appendNode(stack, createNode<EndNode>(), true); }

        stack.resize(stack.size() - toPopCount);
    }

    if(isExclusive) {
        // inserting path of escape for succesful "not matching anything"
        // it'll be replaced with groupEnd during successive appendNode()
        // appending it to OrNode
        appendNode(stack, createNode<EndNode>(), true);
        stack.pop_back();
    }

    appendNode(stack, groupEnd, true);
}

void RegexLexer::adaptOrGroupSymbol(std::vector<dtl::Node*>& stack, std::vector<dtl::Node*>& group, OrGroupMode_t& mode, bool& isRangePending, const char* unit, int ulen, bool& isEscaped) {
    assert(mode != OrGroupMode_t::OUTSIDE);

    if(isRangePending) {
        RangeNode* range = static_cast<RangeNode*>(group.back());
        std::memcpy(range->end, unit, sizeof(range->end));
        range->endlen = ulen;
        isRangePending = false;

        assert(range->startlen == range->endlen);
        return;
    }

    if(!isEscaped && ulen == 1) {
        if(unit[0] == '^' && group.size() == 0) {
            mode = OrGroupMode_t::INSIDE_EXC;
            return;
        }

        if(unit[0] == '-' && group.size() != 0) {
            assert(isNode<UnitNode>(*group.back()));
            char unitBuf[4];
            int unitBufLen;
            {
                UnitNode* unit = static_cast<UnitNode*>(group.back());
                std::memcpy(unitBuf, unit->unit, sizeof(unitBuf));
                unitBufLen = unit->ulen;
                this->nodes.pop_back(); // destroying unit node automatically
            }

            Node* range = createNode<RangeNode>(unitBuf, unitBufLen);
            group.back() = range;
            isRangePending = true;

            return;
        }

        if(unit[0] == ']') {
            appendOrGroupNode(
                stack, group, mode == OrGroupMode_t::INSIDE_EXC);
            mode = OUTSIDE;
            group.clear();
            return;
        }

        if(unit[0] == '\\') {
            isEscaped = true;
            return;
        }
    }

    group.push_back(createNode<UnitNode>(unit, ulen));
    isEscaped = false;
}

void RegexLexer::parsePattern(const std::string& pat) {
    std::vector<GroupNode*> groupStartStack;
    std::vector<Node*> orGroup;
    bool isRangePending = false;

    Children_t stack { nodes[0].get() };
    int ulen = 0;
    char unit[4];
    int unmatchedGroupId = -1;
    bool isEscaped = false;
    int lastRepeatNodeAt = -1;
    OrGroupMode_t orGroupMode = OrGroupMode_t::OUTSIDE;

#if 0
    std::cerr << "START: " << pat << '\n';
#endif 
    
    for(int byteInd = 0; byteInd < pat.size(); byteInd += ulen) {
        ulen = extractUnitStr(unit, pat.c_str() + byteInd);

        Node* newNode = nullptr;

        if(orGroupMode != OUTSIDE) {
            adaptOrGroupSymbol(stack, orGroup, orGroupMode, isRangePending, unit, ulen, isEscaped);
            continue;
        }

        if(ulen > 1) { newNode = createNode<UnitNode>(unit, ulen); }
        else if(isEscaped) { 
            newNode = createNode<UnitNode>(unit, ulen); 
            isEscaped = false;
        }
        else switch(unit[0]) {
        case '|': newNode = createNode<OrNode>(); break;
        case '(': {
            const int gid = this->freeGroupId;
            this->freeGroupId++;
            newNode = createNode<GroupNode>(gid, nullptr, false, true);
            groupStartStack.push_back(static_cast<GroupNode*>(newNode));
        } break;
        case ')': {
            GroupNode* start = groupStartStack.back();
            groupStartStack.pop_back();
            newNode = createNode<GroupNode>(start->groupId, start, true, true);
            start->paired = static_cast<GroupNode*>(newNode);
        } break;
        case '*': {
            newNode = createNode<RepeatNode>(RepeatNode::ZERO_OR_MORE);
            lastRepeatNodeAt = byteInd;
        } break;
        case '+': {
            newNode = createNode<RepeatNode>(RepeatNode::ONE_OR_MORE);
            lastRepeatNodeAt = byteInd;
        } break;
        case '?': {
            assert(byteInd > 0);
            if(lastRepeatNodeAt != byteInd - 1) {
                newNode = createNode<RepeatNode>(RepeatNode::ZERO_OR_ONE);
                lastRepeatNodeAt = byteInd;
            } else {
                static_cast<RepeatNode*>(stack.back())->isLazy = true;
                continue;
            }
        } break;
        case '^': newNode = createNode<AtStartNode>(); break;
        case '$': newNode = createNode<AtEndNode>(); break;
        case '[': orGroupMode = OrGroupMode_t::INSIDE; continue;
        case ']': assert(false && "']' must be handled not here");
        case '\\': isEscaped = true; continue;
        default: newNode = createNode<UnitNode>(unit, ulen); break;
        } // else switch

        appendNode(stack, newNode, true);
    }

    stack.back()->adaptChild(stack, *createNode<EndNode>(), stack.size());

#if 0
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
static bool popUntilFreeChildren(RegexData& data, bool hasLastUnitFetched) {
    // TODO: look at the head of file 
    if(hasLastUnitFetched) { data.returnUnit(); }

    while(data.stack.size() > 1) {
        const NodeMem back = data.stack.back();
        if(back.firstUnprocessedChild < back.node->children.size()) {
            return true;
        }
        if(back.node->usage != UnitUsage_t::NoNeedInUnit) {
            data.returnUnit();
        }
        data.stack.pop_back();
    }

    const NodeMem startNode = data.stack[0];
    if(startNode.firstUnprocessedChild < startNode.node->children.size()) {
        return true;
    }

    data.stack[0].firstUnprocessedChild = 0;
    // proceed by one unit if whole pattern was impossible
    const bool res = data.extractUnit();
    data.startPos += data.ulen;
    return res;
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
    data.str = in.c_str();
    data.strLen = in.length();
    return getToken(out, data);
}

bool RegexLexer::getToken(const char** start, const char** end, RegexData& data) const {
    if(data.at == RegexData::LINE_AT_PAST_EOF) { return false; }

    data.startPos = data.pos;
    data.stack.clear();

    data.groups.insert(data.groups.begin(), this->freeGroupId,
        RegexData::Group{ -1, -1 });

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
                if(!popUntilFreeChildren(data, false)) {
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
            if(!popUntilFreeChildren(data, popUnitIfUnsatisfied)) {
                return false;
            }
            data.reuseUnit = false;
            continue;
        }

        // it's guaranteed that only end node has 0 children
        if(cur->children.size() == 0) {
            if(data.startPos == data.pos) {
                if(data.at == RegexData::LINE_AT_EOF) {
                    data.at = RegexData::LINE_AT_PAST_EOF;
                }
                data.extractUnit();
                data.reuseUnit = false;
                data.startPos = data.pos;
            }

            *start = data.str + data.startPos;
            *end = data.str + data.pos;
            return true;
        }

        // account current child of current parent
        curParent.firstUnprocessedChild += 1;
        data.stack.push_back({cur, next});
    } // while true
    
    return false;
}

bool RegexLexer::getToken(std::string& out, RegexData& data) const {
    const char* start;
    const char* end;
    if(!getToken(&start, &end, data)) { return false; }

    out = std::string(start, end);
    return true;
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
    if(unitLength(str[pos]) == 1 && str[pos] == '\n') { return true; }
    return false;
}

void RegexData::updateAt() {
    if(pos == strLen) { at = LINE_AT_EOF; } 
    //else if(ulen == 1 && unit[0] == '\n') { at = LINE_AT_END; }
    else if(isCurOrNextNewLine()) { at = LINE_AT_END; }
    else { at = LINE_AT_MID; }
}

int RegexData::returnUnit() {
    const int ulen = unitLengthLast(str + pos - 1);
    pos -= ulen;

    updateAt();

    return ulen;
}

bool RegexData::extractUnit() {
    if(pos < strLen) {
        ulen = extractUnitStr(unit, str + pos);
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

    if(isNode<RepeatNode>(node)) {
        insertBetween(stack, node, stack.size() - 1);

        stack.back() = &node;
    } else {
        stack.push_back(&node);
        this->children.push_back(&node);
    }
}

OrNode::OrNode(bool neg)
    : NodeCRTP(
        true,
        neg ? UnitUsage_t::Consume : OrNode::UnitUsage
    ), isNegative(neg) {}

OrNode::OrNode(): OrNode(false) {}

int OrNode::satisfies(RegexData& data) const {
    if(!isNegative) { 
        assert(usage == OrNode::UnitUsage);
        return 0; 
    }
    assert(usage == UnitUsage_t::Consume);
    assert(children.size() > 1);

    for(int i = 0; i < children.size() - 1; ++i) {
        if(children[i]->satisfies(data) != -1) { 
            return -1; 
        }
    }

    return children.size() - 1;
}

void OrNode::adaptEndGroupNode(GroupNode& node, Node& curParent, std::vector<Node*>& visit) {
    const auto found = 
        std::find(visit.cbegin(), visit.cend(), &curParent) != visit.cend();
    if(found) { return; }

    const size_t childrenSize = curParent.children.size();
    assert(curParent.children.size() != 0 && "leaves must have end node");

    Node& back = *curParent.children.back();

    if(auto backEnd = isNode<EndNode>(back); backEnd) {
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
        /*
        adaptEndGroupNode(*gnode, *this, visit);
        gnode->lowerPresedence();
        
        int startAt = gnode->findPairedOnStack(stack);
        stack.resize(startAt + 1);

        stack.push_back(&node);
        */
        stack.push_back(&node);
        dtl::adaptEndGroupNode(*gnode, *this);
        gnode->clearStackBetweenPaired(stack);
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

GroupNode::GroupNode(int id, GroupNode* p, bool isEnd, bool capture)
    : groupId(id)
    , paired(p)
    , capture(capture)
    , NodeCRTP(isEnd) {}

GroupNode::GroupNode(int id, GroupNode* p, bool isEnd): GroupNode(id, p, isEnd, true) {}

bool GroupNode::isEnd() const { return skipSpecials; }

int GroupNode::findPairedOnStack(const Children_t& stack) const {
    assert(stack.size() >= 2 
        && "stack must contain at least start and group start nodes");
    int startAt = stack.size() - 1;

    for(;startAt >= 0; --startAt) {
        if(stack[startAt] == this->paired) { break; }
    }
    assert(startAt >= 1 && "stack must contain group start node");

    return startAt;
}

void GroupNode::clearStackBetweenPaired(Children_t& stack) const {
    assert(this == stack.back() && isEnd() && "group end must be on top");
    const int startAt = findPairedOnStack(stack);
    Node* const nonConstThis = stack.back();
    stack.resize(startAt + 2);
    stack.back() = nonConstThis;
}

int GroupNode::satisfies(RegexData& data) const {
    assert(groupId < static_cast<int>(data.groups.size()));

    if(!capture) { return 0; }

    assert(groupId >= 0 && "only not capturing group may have id < 0");

    if(isEnd()) {
        data.groups[groupId].end = data.pos;
    } else {
        data.groups[groupId].start = data.pos;
    }
    return 0; 
}

void GroupNode::lowerPresedence() {
    assert(paired != nullptr);

    pres = paired->pres = GroupNode::PairedPresedence;
}

void GroupNode::adaptChild(Children_t &stack, Node &node, int at) {
    GroupNode* const gnode = isNode<GroupNode>(node);
    const bool isNodeEnd = gnode != nullptr && gnode->isEnd();

    if(isNode<RepeatNode>(node)) {
        assert(at == stack.size() && "star node must be appended at stack end");
        assert(stack.size() >= 3 && "stack must contain start, group start and group end");

        const int startAt = this->findPairedOnStack(stack);

#if 0
        std::cerr << "STACK BEFORE ENC: ";
        for(auto ch: stack) { std::cerr << NameVisitor::get(*ch) << " "; }
        std::cerr << '\n';
#endif
        
        insertBetween(stack, node, startAt); 
        stack.resize(startAt + 1);
        stack.back() = &node;
    } else if(isNodeEnd && stack.back() != this) { 
#if 0
        std::cerr << "add group end: ";
        for(auto ch: stack) { std::cerr << NameVisitor::get(*ch) << ' '; }
        std::cerr << '\n';
#endif
        assert(stack.size() >= 3 
            && "start, group start and end nodes must be on stack");
        /*
        stack.back() = &node;
        stack[stack.size() - 2]->children.back() = &node;
        */
        /*
        stack.back()->adaptChild(stack, node, stack.size());
        gnode->clearStackBetweenPaired(stack);
        lowerPresedence();
        */
        dtl::adaptEndGroupNode(*gnode, *this);
        stack.push_back(gnode);
        gnode->clearStackBetweenPaired(stack);
        lowerPresedence();
    } else {
        insertBetween(stack, node, at);
        stack.resize(at);
        stack.push_back(&node);
        if(isNodeEnd) { lowerPresedence(); }
    }

#if 0
    std::cerr << "AFTER ENCOMPASSING REPEAT\n";
    std::vector<Node*> tr;
    print(stack[0], 0, tr);
    std::cerr << "END REPEAT\n";
#endif
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

RepeatNode::RepeatNode(Mode mode): mode(mode) {}

int RepeatNode::satisfies(RegexData& data) const { return 0; }

void RepeatNode::adaptChild(Children_t &stack, Node &node, int at) {
    assert(this->children.size() == 1);

    Node& firstChild = *this->children.back();
    Node* subAdapter = &firstChild;

    if(isNode<GroupNode>(firstChild)) {
        subAdapter = static_cast<Node*>(
            static_cast<GroupNode&>(firstChild).paired);
    }

    if(mode == ZERO_OR_ONE) {
        if(isLazy) {
            this->children.insert(this->children.begin(), &node);
        } else {
            this->children.push_back(&node);
        }
    } else if(mode == ZERO_OR_MORE) {
        if(isLazy) {
            this->children.insert(this->children.begin(), &node);
        } else {
            this->children.push_back(&node);
        }
        subAdapter->children.push_back(this); 
    } else {
        if(isLazy) {
            // pass
        } else {
            subAdapter->children.push_back(this); 
        }
    }

    subAdapter->children.push_back(&node); 
    stack.push_back(&node);

#if 0
    std::cerr << "MYMYMYMYMYMMYMYM\n";
    std::vector<Node*> tr;
    print(stack[0], 0, tr);
    std::cerr << "END MYMYMYMYMYMMYMYM\n";
#endif
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

RangeNode::RangeNode() {}

RangeNode::RangeNode(const char* start, int startlen): startlen(startlen) {
    std::memcpy(this->start, start, sizeof(this->start));
}

RangeNode::RangeNode(const char* start, const char* end, int startlen, int endlen): startlen(startlen), endlen(endlen) {
    assert(startlen == endlen);

    std::memcpy(this->start, start, sizeof(this->start));
    std::memcpy(this->end, end, sizeof(this->end));
}

int RangeNode::satisfies(RegexData& data) const {
    assert(startlen == endlen);

    if(data.ulen != startlen
    || std::memcmp(start, data.unit, startlen) > 0
    || std::memcmp(end, data.unit, startlen) < 0) { return -1; }
    return 0;
}

void RangeNode::adaptChild(Children_t &stack, Node &node, int at) {
    assert(at == stack.size() && "child of RangeNode must be only appended");

    if(isNode<RepeatNode>(node)) {
        insertBetween(stack, node, stack.size() - 1);

        stack.back() = &node;
    } else {
        stack.push_back(&node);
        this->children.push_back(&node);
    }
}

FailNode::FailNode() {}

int FailNode::satisfies(RegexData& data) const { 
    return -1;
}

void FailNode::adaptChild(Children_t &stack, Node &node, int at) {
    assert(at == stack.size() && "FailNode's child must be appended");

    stack.push_back(&node);
    this->children.push_back(&node);
}

} // namespace dlexer
