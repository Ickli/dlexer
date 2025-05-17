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
struct StarNode;
struct EndNode;

using Children_t = std::vector<Node*>;

enum UnitUsage_t {
    Consume,
    ShareWithChild,
    NoNeedInUnit,
};

struct Node {
    Children_t children;
    int presedence;
    UnitUsage_t usage;

    Node(int pres, UnitUsage_t usage)
        : presedence(pres)
        , usage(usage)
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
    virtual void visit(StarNode& node) {}
    virtual void visit(EndNode& node) {}
};

template<typename Derived>
struct NodeCRTP: Node {
    NodeCRTP(): Node(Derived::Presedence, Derived::UnitUsage) {}

    void acceptVisitor(dtl::INodeVisitor& visitor) override {
        Derived& castedSelf = *static_cast<Derived*>(this);
        visitor.visit(castedSelf);
    }
};

struct UnitNode: NodeCRTP<UnitNode> {
    static const int Presedence = 1;
    static const UnitUsage_t UnitUsage = UnitUsage_t::Consume;
    char unit[4];
    int ulen;

    UnitNode(const char* unitPtr, int ulen);

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct StartNode: NodeCRTP<StartNode> {
    static const int Presedence = 6;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct GroupNode: NodeCRTP<GroupNode> {
    static const int Presedence = 1;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;
    int groupId;
    // != nullptr only if it's a start node, otherwise == nullptr
    GroupNode* paired;

    GroupNode(int id, GroupNode* paired);

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct OrNode: NodeCRTP<OrNode> {
    static const int Presedence = 5;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;

    OrNode();

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct StarNode: NodeCRTP<StarNode> {
    static const int Presedence = 1;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;

    StarNode();

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct EndNode: NodeCRTP<EndNode> {
    static const int Presedence = 1;
    static const UnitUsage_t UnitUsage = UnitUsage_t::NoNeedInUnit;

    EndNode();

    int satisfies(RegexData& data) const override;
    void adaptChild(Children_t& stack, Node& node, int at) override;
};

struct NodeMem {
    dtl::Node* node;
    int firstUnprocessedChild;
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

    std::string* encodeAlpha(dtl::Children_t& stack, const char* unit, int ulen);
    std::string* encodeNonAlpha(dtl::Children_t& stack, char unit);
    std::string* encodeOr(dtl::Children_t& stack);
    std::string* encodeStartCaptureGroup(dtl::Children_t& stack);
    std::string* encodeEndCaptureGroup(dtl::Children_t& stack);

    void parsePattern(const std::string& pat);

    template<typename NodeType, typename... Args>
    dtl::Node* createNode(Args... args) {
        nodes.push_back(std::make_unique<NodeType>(std::forward<Args>(args)...));
        return nodes.back().get();
    }
};
} // namespace dlexer
#endif // DLEXER_REGEX_H_
