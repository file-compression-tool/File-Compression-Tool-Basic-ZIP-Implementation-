
#include "huffman_tree.hpp"

#include <cstdint>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
    struct CompareNodes
    {
        bool operator()(
            const std::shared_ptr<HuffmanNode>& a,
            const std::shared_ptr<HuffmanNode>& b) const
        {
            if (a->frequency != b->frequency)
                return a->frequency > b->frequency;

            return a->symbol > b->symbol;
        }
    };
}

std::shared_ptr<HuffmanNode> buildHuffmanTree(
    const std::map<uint8_t, std::uint64_t>& frequencies)
{
    if (frequencies.empty())
        return nullptr;

    std::priority_queue<
        std::shared_ptr<HuffmanNode>,
        std::vector<std::shared_ptr<HuffmanNode>>,
        CompareNodes> queue;

    for (const auto& entry : frequencies)
    {
        if (entry.second == 0)
        {
            throw std::invalid_argument(
                "Symbol frequency must be greater than zero.");
        }

        queue.push(std::make_shared<HuffmanNode>(
            entry.first, entry.second));
    }

    while (queue.size() > 1)
    {
        auto left = queue.top();
        queue.pop();

        auto right = queue.top();
        queue.pop();

        if (left->frequency > UINT64_MAX - right->frequency)
        {
            throw std::overflow_error(
                "Huffman frequency total overflow.");
        }

        auto parent = std::make_shared<HuffmanNode>(
            left->symbol,
            left->frequency + right->frequency);

        parent->left = std::move(left);
        parent->right = std::move(right);

        queue.push(std::move(parent));
    }

    return queue.top();
}

HuffmanCodeMap generateHuffmanCodes(
    const std::shared_ptr<HuffmanNode>& root)
{
    HuffmanCodeMap codes;

    if (!root)
        return codes;

    struct PendingNode
    {
        std::shared_ptr<HuffmanNode> node;
        std::string prefix;
    };

    std::vector<PendingNode> pending;
    pending.push_back({root, ""});

    while (!pending.empty())
    {
        PendingNode current = std::move(pending.back());
        pending.pop_back();

        if (current.node->isLeaf())
        {
            codes[current.node->symbol] =
                current.prefix.empty() ? "0" : std::move(current.prefix);
            continue;
        }

        if (!current.node->left || !current.node->right)
        {
            throw std::invalid_argument(
                "Internal Huffman node must have both children.");
        }

        std::string rightPrefix = current.prefix;
        rightPrefix.push_back('1');

        current.prefix.push_back('0');

        pending.push_back({
            current.node->right,
            std::move(rightPrefix)
        });

        pending.push_back({
            current.node->left,
            std::move(current.prefix)
        });
    }

    return codes;
}
