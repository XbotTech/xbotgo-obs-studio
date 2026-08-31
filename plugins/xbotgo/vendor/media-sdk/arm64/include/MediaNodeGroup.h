#pragma once

#include "DataTypes.h"
#include "MediaNodeBase.h"
#include "worker_thread.h"
#include <map>
#include <set>
#include <string>
#include <vector>

namespace blink {
namespace media {

struct MediaNodeLink {
    int32_t fromNodeId;
    MEDIA_DATA_TYPE mediaType;
    int32_t toNodeId;

    MediaNodeLink() : fromNodeId(-1), mediaType(MEDIA_DATA_TYPE_UNKNOWN), toNodeId(-1) {}
};

// MediaNodeGroup is not thread safe, should be created and used in one thread
// MediaNodeGroup stand for a group of MediaNodes with stable nodes and internal links
// Input and output links from outside to node in MediaNodeGroup can be added by demand
struct MediaNodeGroup {
    struct OutLinkCmp {
        bool operator()(const MediaNodeLink &a, const MediaNodeLink &b) const {
            if (a.toNodeId != b.toNodeId) {
                return a.toNodeId < b.toNodeId;
            }
            if (a.mediaType != b.mediaType) {
                return a.mediaType < b.mediaType;
            }
            return true;
        }
    };
    struct InLinkCmp {
        bool operator()(const MediaNodeLink &a, const MediaNodeLink &b) const {
            if (a.fromNodeId != b.fromNodeId) {
                return a.fromNodeId < b.fromNodeId;
            }
            if (a.mediaType != b.mediaType) {
                return a.mediaType < b.mediaType;
            }
            return true;
        }
    };

    typedef std::set<MediaNodeLink, OutLinkCmp> OutLinkSet;
    typedef std::set<MediaNodeLink, InLinkCmp> InLinkSet;

    struct VertexInfo {
        std::unique_ptr<MediaNode> node;
        OutLinkSet outLinks;
        InLinkSet inLinks;
        
        VertexInfo() {}
        VertexInfo(std::unique_ptr<MediaNode> n, OutLinkSet out, InLinkSet in) : 
            node(std::move(n)), outLinks(out), inLinks(in) {}
    };

    std::shared_ptr<worker_thread> m_worker;
    std::unordered_map<int32_t, VertexInfo> m_vertices;
    std::map<std::string, VertexInfo*> m_namesToVertex;

    MediaNodeGroup(int32_t *nodeId, MessageListener *listener, std::shared_ptr<worker_thread> worker = nullptr, bool attach_to_java_thread = false);
    ~MediaNodeGroup();

    int create(std::vector<MediaNodeConfig> &nodeConfigs,
        std::vector<MediaNodeLink> &links);
    int destroy();
    MediaNode* createNode(const std::string &type, std::shared_ptr<GenericData> config);

    /**
     * Connect two nodes with a media link
     * @param fromNodeId Source node ID
     * @param mediaType Media data type for the connection
     * @param toNodeId Destination node ID
     * @return 0 on success, -1 on failure
     */
    int connectNode(int32_t fromNodeId, MEDIA_DATA_TYPE mediaType, int32_t toNodeId);

    /**
     * Disconnect two nodes
     * @param fromNodeId Source node ID
     * @param mediaType Media data type of the connection
     * @param toNodeId Destination node ID
     * @return 0 on success, -1 on failure
     */
    int disconnectNode(int32_t fromNodeId, MEDIA_DATA_TYPE mediaType, int32_t toNodeId);

    /**
     * Destroy a node and disconnect all its connections
     * @param nodeId Node ID to destroy
     * @return 0 on success, -1 on failure
     */
    int destroyNode(int32_t nodeId);

    /**
     * Destroy a node tree using breadth-first traversal
     * Destroys the specified node and all its descendant nodes (nodes reachable via outgoing links)
     * Nodes that are still connected by other nodes (outside the tree) will not be destroyed
     * @param rootNodeId Root node ID of the tree to destroy
     * @return 0 on success, -1 on failure
     */
    int destroyNodeTree(int32_t rootNodeId);

    MediaNode* getNodeById(int32_t nodeId);
    MediaNode* getNodeByName(const std::string &name);
    std::vector<MediaNode*> getNodesByType(const std::string &type);
private:
    int32_t *m_nodeId;
    MessageListener *m_listener;
};

}
}
