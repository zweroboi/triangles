#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class NodeRef;

//
// Triangle

class Triangle {
    friend class NodeRef;
    friend std::istream& operator>>(std::istream& is, Triangle& triangle);

public:
    Triangle()
      : height_(0),
        order_(0)
    {}

    bool empty() const { return items_.empty(); }
    size_t num_items() const { return items_.size(); }
    size_t height() const { return height_; }

    size_t row_size(size_t level) const;

    NodeRef head() const;

private:
    std::vector<int> items_;  // Triangle items storage.
    size_t height_;           // Triangle height.
    size_t order_;            // Number of child node references for a single node.
};

//
// Triangle node reference

class NodeRef {
    friend class Triangle;

public:
    int id() const { return id_; }
    int value() const { return triangle_->items_[id_]; }
    size_t level() const { return level_; }
    bool final() const { return level_ == triangle_->height_ - 1; }

    size_t num_children() const {
        if (!final()) {
            return triangle_->order_;
        }
        return 0;
    }

    NodeRef child(size_t n) const {
        if (n >= num_children()) {
            std::ostringstream oss;
            oss << "node #" << id_ << " doesn't have child #" << n;
            throw std::out_of_range(oss.str());
        }
        size_t child_id = id_ + triangle_->row_size(level_) + n;
        return NodeRef(child_id, 1 + level_, triangle_);
    }

private:
    NodeRef(size_t id, size_t level, const Triangle* triangle)
      : id_(id),
        level_(level),
        triangle_(triangle)
    {}

private:
    const Triangle* triangle_;
    size_t id_;
    size_t level_;
};

//
// Triangle methods and I/O implementation

NodeRef Triangle::head() const {
    if (!empty()) {
        return NodeRef(0, 0, this);
    }
    throw std::out_of_range("triangle is empty");
}

size_t Triangle::row_size(size_t level) const {
    if (order_ > 0) {
        return 1 + level * (order_ - 1);
    }
    return 0;
}

std::istream& operator>>(std::istream& is, Triangle& triangle) {
    std::string line;
    size_t expected = 1;  // Expected number of integers in next read string.
    while (std::getline(is, line)) {
        if (line.empty()) {
            break;
        }
        triangle.height_++;
        std::istringstream iss(line);
        size_t count = 0;
        int item;
        while (iss >> item || !iss.eof()) {
            if (iss.fail()) {
                iss.clear();
                std::string item;
                iss >> item;
                std::ostringstream oss;
                oss << "at line " << triangle.height_ << ": can't parse integer: " << item;
                throw std::runtime_error(oss.str());
            }
            triangle.items_.push_back(item);
            count++;
        }
        if (triangle.height_ == 2) {
            triangle.order_ = count;  // Line 2 should contain the number of children for a single node.
        } else if (count != expected) {
            std::ostringstream oss;
            oss << "at line " << triangle.height_ << ": expected " << expected << " items, got " << count;
            throw std::runtime_error(oss.str());
        }
        expected = triangle.row_size(triangle.height_);
    }
    return is;
}

//
// Max path sum algorithm implementattion

int max_path_sum(const Triangle& triangle) {
    if (triangle.empty()) {
        return 0;
    }

    size_t cache_size = triangle.num_items() - triangle.row_size(triangle.height() - 1);
    std::vector<int> max_path_cache(cache_size);
    std::vector<bool> max_path_cached(cache_size, false);

    NodeRef head = triangle.head();

    std::vector<NodeRef> path;
    std::vector<size_t> next_child;
    std::vector<int> max_child_path_sum;

    path.push_back(head);
    next_child.push_back(0);
    max_child_path_sum.push_back(0);

    int max_path_sum;

    while (!path.empty()) {
        if (next_child.back() == path.back().num_children()) {
            int max_curr_path_sum = path.back().value() + max_child_path_sum.back();
            max_path_cached[path.back().id()] = true;
            max_path_cache[path.back().id()] = max_curr_path_sum;
            max_child_path_sum.pop_back();
            next_child.pop_back();
            path.pop_back();
            if (!max_child_path_sum.empty()) {
                if (max_curr_path_sum > max_child_path_sum.back() || next_child.back() == 0) {
                    max_child_path_sum.back() = max_curr_path_sum;
                }
            } else {
                max_path_sum = max_curr_path_sum;
            }
            continue;
        }
        NodeRef child = path.back().child(next_child.back());
        if (child.final()) {
            int child_value = child.value();
            if (child_value > max_child_path_sum.back() || next_child.back() == 0) {
                max_child_path_sum.back() = child_value;
            }
            next_child.back()++;
            continue;
        }
        if (max_path_cached[child.id()]) {
            int child_max_path = max_path_cache[child.id()];
            if (child_max_path > max_child_path_sum.back() || next_child.back() == 0) {
                max_child_path_sum.back() = child_max_path;
            }
            next_child.back()++;
            continue;
        }
        next_child.back()++;

        path.push_back(child);
        next_child.push_back(0);
        max_child_path_sum.push_back(0);
    }

    return max_path_sum;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cerr << "Usage: " << argv[0] << " input_file" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    std::ifstream input(filename);
    if (!input) {
        std::cerr << "Can't open file: " << filename << std::endl;
        return 2;
    }

    Triangle triangle;
    int max_sum;
    try {
        std::cerr << "Reading triangle from " << filename << "... " << std::flush;
        input >> triangle;
        std::cerr << "done" << std::endl;
        std::cerr << "Processing... " << std::flush;
        max_sum = max_path_sum(triangle);
        std::cerr << "done" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 2;
    }

    std::cout << "Max path sum: " << max_sum << std::endl;

    return 0;
}
