#ifndef DLEXER_REGEX_H_
#define DLEXER_REGEX_H_
#include <vector>
#include <string>
#include <memory>
#include <utility>

namespace dlexer {

struct RegexData;

namespace dtl {

struct INodeVisitor;
struct Node;
struct UnitNode;
struct StartNode;
struct GroupNode;
struct OrNode;
struct RepeatNode;
struct EndNode;
struct AtStartNode;
struct AtEndNode;
struct RangeNode;
struct FailNode;
struct OrGroupNode;

using Children_t = std::vector<Node*>;

enum UnitUsage_t {
    Consume,
    ShareWithChild,
    NoNeedInUnit,
};

struct Node {
    Children_t children;
    UnitUsage_t usage;
    int pres;
    bool skipSpecials;

    Node(bool skip, UnitUsage_t usage, int pres)
        : skipSpecials(skip)
        , usage(usage)
        , pres(pres)
        {}
    virtual ~Node() {}
    virtual int satisfies(RegexData& data) const = 0;

    virtual void acceptVisitor(dtl::INodeVisitor& visitor) = 0;

    // returns next child adapter
    virtual void adaptChild(Children_t& stack, Node& node, int at) = 0;
};

struct INodeVisitor {
    // Not virtual, because not intented to be stored in a container
    // ~INodeVisitor();

    virtual void visit(UnitNode& node) {}
    virtual void visit(StartNode& node) {}
    virtual void visit(GroupNode& node) {}
    virtual void visit(OrNode& node) {}
    virtual void visit(RepeatNode& node) {}
    virtual void visit(EndNode& node) {}
    virtual void visit(AtStartNode& node) {}
    virtual void visit(AtEndNode& node) {}
    virtual void visit(RangeNode& node) {}
    virtual void visit(FailNode& node) {}
};

template<typename Derived>
struct NodeCRTP: Node {
    NodeCRTP(): Node(Derived::SkipSpecials, Derived::UnitUsage, Derived::Presedence) {}
    NodeCRTP(bool skip, UnitUsage_t usage): Node(skip, usage, Derived::Presedence) {}
    NodeCRTP(bool skip): NodeCRTP(skip, Derived::UnitUsage) {}

    void acceptVisitor(dtl::INodeVisitor& visitor) override {
        Derived& castedSelf = *static_cast<Derived*>(this);
        visitor.visit(castedSelf);
    }
};

struct UnitNode: NodeCRTP<UnitNode> {
    static const int Presedence = 1;
    static const bool SkipSpecials = false;
    static const UnitUsage_t UnitUsage = UnitUsage_t::Consume;
    char unit[4];
    int ulen;

    UnitNode(const char* unitPtr, int ulen);

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct StartNode: NodeCRTP<StartNode> {
    static const int Presedence = 6;
    static const bool SkipSpecials = false;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct GroupNode: NodeCRTP<GroupNode> {
    static const int Presedence = 2;
    static const int PairedPresedence = 1;
    static const bool SkipSpecials = true;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;
    GroupNode* paired;
    int groupId;
    bool capture;

    GroupNode(int id, GroupNode* paired, bool isEnd, bool capture);
    GroupNode(int id, GroupNode* paired, bool isEnd);

    bool isEnd() const;

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;

    void lowerPresedence();
};

struct OrNode: NodeCRTP<OrNode> {
    static const int Presedence = 2;
    static const bool SkipSpecials = true;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;

    // If true, becomes consuming;
    // If true, checks that every child except the last one is not satisfied;
    //      If so, returns index of the last child; otherwise, returns -1.
    bool isNegative;

    OrNode(bool neg);
    OrNode();

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;

private:
    static void adaptEndGroupNode(GroupNode& node, Node& curParent, std::vector<Node*>& visited);
};

struct RepeatNode: NodeCRTP<RepeatNode> {
    static const int Presedence = 1;
    static const bool SkipSpecials = false;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;

    bool atLeastOne;
    
    RepeatNode(bool atLeastOne);

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct EndNode: NodeCRTP<EndNode> {
    static const int Presedence = 1;
    static const bool SkipSpecials = false;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;

    EndNode();

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct AtStartNode: NodeCRTP<AtStartNode> {
    static const int Presedence = 1;
    static const bool SkipSpecials = false;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;

    AtStartNode();

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct AtEndNode: NodeCRTP<AtEndNode> {
    static const int Presedence = 1;
    static const bool SkipSpecials = false;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;

    AtEndNode();

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct RangeNode: NodeCRTP<RangeNode> {
    static const int Presedence = 1;
    static const bool SkipSpecials = false;
    static const UnitUsage_t UnitUsage = UnitUsage_t::Consume;

    int startlen;
    int endlen;
    unsigned char start[4];
    unsigned char end[4];

    RangeNode();
    RangeNode(const char* start, int startlen);
    RangeNode(const char* start, const char* end, int startlen, int endlen);

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct FailNode: NodeCRTP<FailNode> {
    static const int Presedence = 1;
    static const bool SkipSpecials = false;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;

    FailNode();

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct NodeMem {
    dtl::Node* node;
    int firstUnprocessedChild;
};

enum OrGroupMode_t {
    OUTSIDE,
    INSIDE,
    INSIDE_EXC,
};
} // namespace dtl

struct RegexData {
    std::vector<dtl::NodeMem> stack;
    const std::string* str = nullptr;
    char unit[4];
    int ulen = 0;
    int pos = 0;
    enum {
        LINE_AT_START,
        LINE_AT_MID,
        LINE_AT_END,
        LINE_AT_EOF,
        LINE_AT_PAST_EOF,
    } at = LINE_AT_START;
    bool reuseUnit = false;

    bool extractUnit();
    // returns number of bytes of reverted unit
    int returnUnit();
    void updateAt();

    // WARNING: doesn't check for eof
    bool isCurOrNextNewLine() const;
};

class RegexLexer {
public:
    std::string* err = nullptr;
    RegexData data;

    RegexLexer(const std::string& pat);

    bool getToken(std::string& out, std::istream& in);
    bool getToken(std::string& out, const std::string& in);
    bool getToken(std::string& out, RegexData& data) const;
    void reprogram(const std::string& pat);
private:
    struct GroupData {
        int start;
        int end;
    };

    std::vector<std::unique_ptr<dtl::Node>> nodes;
    std::vector<GroupData> groups;
    std::istream* istream = nullptr;
    std::string istreamString;

    void extractStringFromIstream(std::istream& s);

    void parsePattern(const std::string& pat);
    void appendNode(dtl::Children_t& stack, dtl::Node* newNode);
    void appendOrGroupNode(dtl::Children_t& stack, std::vector<dtl::Node*> orGroup, bool isExclusive);
    void adaptOrGroupSymbol(std::vector<dtl::Node*>& stack, std::vector<dtl::Node*>& group, dtl::OrGroupMode_t& mode, bool& isRangePending, const char* unit, int ulen, bool& isEscaped);

    template<typename NodeType, typename... Args>
    dtl::Node* createNode(Args... args) {
        nodes.push_back(std::make_unique<NodeType>(std::forward<Args>(args)...));
        return nodes.back().get();
    }
};
} // namespace dlexer
#endif // DLEXER_REGEX_H_
