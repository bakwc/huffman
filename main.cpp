#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <unordered_map>

using namespace std;

struct TNode;
typedef shared_ptr<TNode> TNodeRef;
struct TNode {
    char Value;
    size_t Freq;
    TNodeRef Left;
    TNodeRef Right;
};

typedef vector<bool> TBits;
typedef unordered_map<char, TBits> TCompressTable;

TBits ToBits(char element) {
    TBits result;
    result.reserve(8);
    for (size_t i = 0; i < 8; ++i) {
        char current = element;
        current <<= i;
        current >>= 8;
        result.push_back(current);
    }
    return result;
}

char FromBits(const TBits& bits, size_t offset) {
    char result = 0;
    for (size_t i = offset; i < min(size_t(offset + 8), bits.size()); ++i) {
        char e = bits[i];
        e <<= (7 - (i - offset));
        result |= e;
    }
    return result;
}

std::string BitsToString(const TBits& bits) {
    string result;
    result.reserve(bits.size() / 8 + 1);
    size_t i;
    for (i = 0; i < bits.size(); i += 8) {
        result.push_back(FromBits(bits, i));
    }
    result.push_back(i - bits.size());
    return result;
}

TBits StringToBits(const std::string& data) {
    TBits bits;
    bits.reserve(data.size() * 8);
    for (size_t i = 0; i < data.size() - 1; ++i) {
        TBits currentBits = ToBits(data[i]);
        bits.insert(bits.end(), currentBits.begin(), currentBits.end());
    }
    unsigned char nullsCount = data[data.size() - 1];
    for (size_t i = 0; i < nullsCount; ++i) {
        bits.pop_back();
    }
    return bits;
}

struct THuffmanCompressor {
public:
    string Compress(const string& data) {
        InitList(data);
        while (FreeList.size() > 1) {
            JoinMin();
        }
        TCompressTable table;
        GetTable(FreeList.begin()->second, table, TBits());
        TBits compressed = SerializeTree(FreeList.begin()->second);

        for (size_t i = 0; i < data.size(); ++i) {
            TBits& bits = table[data[i]];
            compressed.insert(compressed.end(), bits.begin(), bits.end());
        }
        return BitsToString(compressed);
    }

    string Decompress(const string& data) {
        TBits compressed = StringToBits(data);

        size_t offset = 0;
        TNodeRef root = make_shared<TNode>();
        ConstructTree(root, compressed, offset);

        TNodeRef node = root;
        string result;
        for (size_t i = offset; i < compressed.size(); ++i) {
            if (compressed[i]) {
                node = node->Left;
            } else {
                node = node->Right;
            }
            if (!node->Right) {
                result.push_back(node->Value);
                node = root;
            }
        }
        return result;
    }
private:
    void InitList(const string& data) {
        unordered_map<char, size_t> freqTable;
        for (size_t i = 0; i < data.size(); ++i) {
            ++freqTable[data[i]];
        }
        for (auto it = freqTable.begin(); it != freqTable.end(); ++it) {
            TNodeRef node = make_shared<TNode>();
            node->Value = it->first;
            node->Freq = it->second;
            Add(node);
        }
    }
    void JoinMin() {
        TNodeRef a = PopMin();
        TNodeRef b = PopMin();
        TNodeRef c = make_shared<TNode>();
        c->Freq = a->Freq + b->Freq;
        if (a->Freq < b->Freq) {
            c->Left = a;
            c->Right = b;
        } else {
            c->Left = b;
            c->Right = a;
        }
        Add(c);
    }
    void GetTable(TNodeRef node, TCompressTable& table, TBits currentBits) {
        if (node->Left) {
            currentBits.push_back(true);
            GetTable(node->Left, table, currentBits);
            currentBits[currentBits.size() - 1] = !currentBits[currentBits.size() - 1];
            GetTable(node->Right, table, currentBits);
        } else {
            table[node->Value] = currentBits;
        }
    }
    TBits SerializeTree(TNodeRef node) {
        TBits result;
        if (node->Left) {
            TBits left = SerializeTree(node->Left);
            TBits right = SerializeTree(node->Right);
            result.reserve(left.size() + right.size() + 1);
            result.push_back(0);
            result.insert(result.end(), left.begin(), left.end());
            result.insert(result.end(), right.begin(), right.end());
        } else {
            result.reserve(9);
            TBits valueBits = ToBits(node->Value);
            result.push_back(1);
            result.insert(result.end(), valueBits.begin(), valueBits.end());
        }
        return result;
    }
    void ConstructTree(TNodeRef node, const TBits& bits, size_t& offset) {
        char type = bits[offset];
        ++offset;
        if (type == 0) {
            node->Left = make_shared<TNode>();
            ConstructTree(node->Left, bits, offset);
            node->Right = make_shared<TNode>();
            ConstructTree(node->Right, bits, offset);
        } else {
            node->Value = FromBits(bits, offset);
            offset += 8;
        }
    }

    void Add(TNodeRef node) {
        FreeList.insert(pair<size_t, TNodeRef>(node->Freq, node));
    }
    TNodeRef PopMin() {
        auto it = FreeList.begin();
        TNodeRef element = it->second;
        FreeList.erase(it);
        return element;
    }
private:
    multimap<size_t, TNodeRef> FreeList;
};

string Compress(const string& data) {
    THuffmanCompressor compressor;
    return compressor.Compress(data);
}

string Decompress(const string& compressed) {
    THuffmanCompressor compressor;
    return compressor.Decompress(compressed);
}

int main() {
    string data = "Airlines jet returned to the Dallas airport safely Friday after striking a flock of birds shortly after takeoff, officials said. The Federal Aviation Administration said the plane sustained minor injuries, the Dallas Morning News reported. The flight left Dallas/Fort Worth International Airport about 6:20 p.m. bound for Ronald Reagan Washington National Airport when the incident occurred. The pilot declared an emergency after encountering the birds and quickly returned to the Dallas airport for \"precautionary measures,\" the FAA said. The newspaper said it wasn't clear how many people were aboard the plane. Read more: http://www.upi.com/Top_News/US/2014/02/28/American-Airlines-jetliner-hits-flock-of-birds-on-takeoff-in-Dallas/UPI-55411393644657/#ixzz2ui4PDxyd";
    string compressed = Compress(data);
    string decompressed = Decompress(compressed);

    cout << "data size:  " << data.size() << "\n";
    cout << "compressed: " << compressed.size() << "\n";
    cout << "rate:       " << float(compressed.size()) / data.size() << "\n";

    return 0;
}
