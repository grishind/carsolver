#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <unordered_set>
#include <vector>

using std::vector;

std::string AddBrackets(const std::string& content,
                        const std::string& open,
                        const std::string& close,
                        const std::string& tabs) {
    const std::string newtabs = tabs + "  ";
    return
            open + "\n" +
            content + "\n" +
            close;
}

size_t Combine(size_t hash1, size_t hash2)
{
    return (hash1 ^ (hash2 << 1)) >> 1;
}

enum class Dir {
    Down,
    Right
};


std::string StringifyDir(Dir dir) {
    return (dir == Dir::Down ? "\"down\"" : "\"right\"");
}

struct Point {
    int x, y;

    size_t Hash() const
    {
        const auto hash_x = std::hash<int>()(x);
        const auto hash_y = std::hash<int>()(y);
        return Combine(hash_x, hash_y);
    }

    bool operator==(const Point& rhs) const { return x == rhs.x && y == rhs.y; }
};

struct Finish {
    Point pos;
    Dir dir;
};

struct Field {
    int width;
    int height;
    Finish fin;
};

std::string StringifyField(const Field& field) {
    return std::string("\"field\": {\n") +
            "\"width\": " + std::to_string(field.width) + ",\n" +
            "\"height\": " + std::to_string(field.height) + "\n" +
            "}";
}

struct Car {
    int size;
    Dir dir;
    Point pos;

    vector<Point> Place() const
    {
        vector<Point> placement;
        for (int i = 0; i < size; ++i) {
            placement.push_back({
                pos.x + i * (dir == Dir::Right),
                pos.y + i * (dir == Dir::Down)
            });
        }
        return placement;
    }

    Car Move(int step) const
    {
        return { size,
            dir,
            { pos.x + step * (dir == Dir::Right),
                pos.y + step * (dir == Dir::Down) } };
    }

    bool Contains(const Point& point) const
    {
        const vector<Point> placement = Place();
        return std::find(placement.begin(), placement.end(), point) != placement.end();
    }

    size_t Hash() const
    {
        const auto hash1 = std::hash<int>()(size);
        const auto hash2 = std::hash<int>()(static_cast<int>(dir));
        const auto hash3 = pos.Hash();
        return Combine(hash1, Combine(hash2, hash3));
    }

    bool operator==(const Car& rhs) const
    {
        return size == rhs.size && dir == rhs.dir && pos == rhs.pos;
    }

    std::string Stringify(const std::string& tabs = "") const {
        return AddBrackets(
                tabs + "\"size\": " + std::to_string(size) + ",\n" +
                tabs + "\"dir\": " + StringifyDir(dir) + ",\n" +
                tabs + "\"x\": " + std::to_string(pos.x) + ",\n" +
                tabs + "\"y\": " + std::to_string(pos.y) + "\n",
                    "{", "}", tabs + "  ");
    };
};

std::string StringifyCars(const std::vector<Car>& cars, const std::string& tabs = "") {
    std::string res = "";
    for (size_t i = 0; i < cars.size(); ++i) {
        res += cars.at(i).Stringify();
        if (i + 1 < cars.size()) {
            res += ",\n";
        }
    }
    return AddBrackets(res, "[", "]", tabs);
}

struct Scene {
    Field field;
    vector<Car> cars;
    vector<vector<int>> hitmap;

    const Car& Hero() const { return cars.front(); }

    Scene(const Field& field, const vector<Car>& cars)
        : field(field)
        , cars(cars)
        , hitmap(CreateHitmap(field.width, field.height))
    {
    }

    size_t Hash() const
    {
        size_t h = 0;
        for (const auto& car : cars) {
            h = Combine(h, car.Hash());
        }
        return h;
    }

    bool operator==(const Scene& rhs) const { return cars == rhs.cars; }

    bool IsTerminal() const { return Hero().Contains(field.fin.pos); }

    bool IsProper() const
    {
        for (const auto& rows : hitmap) {
            for (const auto& hit_count : rows) {
                if (hit_count > 1) {
                    return false;
                }
            }
        }
        return true;
    }

    vector<Scene> GetNeighbours() const
    {
        vector<Scene> neighbours;
        for (size_t i = 0; i < cars.size(); ++i) {
            vector<Car> forward_cars(cars);
            vector<Car> backward_cars(cars);
            Car& fcar = forward_cars.at(i);
            Car& bcar = backward_cars.at(i);
            fcar = fcar.Move(1);
            bcar = bcar.Move(-1);
            Scene scene_forward(field, forward_cars);
            Scene scene_backward(field, backward_cars);
            const vector<Scene> candidates { scene_forward, scene_backward };
            for (const Scene& cand : candidates) {
                if (cand.IsProper()) {
                    neighbours.push_back(cand);
                }
            }
        }
        return neighbours;
    }

private:
    static int Cap(int value, int bound)
    {
        if (value >= bound) {
            return bound - 1;
        }
        if (value < 0) {
            return 0;
        }
        return value;
    }

    vector<vector<int>> CreateHitmap(int width, int height) const
    {
        vector<vector<int>> hit(height, vector<int>(width, 0));
        for (const auto& car : cars) {
            const auto placement = car.Place();
            for (const auto& point : placement) {
                hit.at(Cap(point.y, height))
                   .at(Cap(point.x, width)) += 1;
            }
        }
        return hit;
    }
};

struct Input {
    Field field;
    Scene scene;
};

struct SceneHasher {
    std::size_t operator()(const Scene& scene) const { return scene.Hash(); }
};

using HSet = std::unordered_set<Scene, SceneHasher>;

class Solver {
public:
    Solver(const Field& field)
        : field(field)
        , hset()
    {
    }
    vector<Scene> Solve(const Scene& scene)
    {
        struct Node;
        using NodePtr = std::shared_ptr<Node>;
        struct Node {
            Scene scene;
            NodePtr parent;
        };

        auto root = std::make_shared<Node>(Node{scene, nullptr});
        std::deque<NodePtr> queue;
        queue.push_back(root);
        while (!queue.empty()) {
            const auto &current = queue.front();

            if (current->scene.IsTerminal()) {
                vector<Scene> path;
                auto node = current;
                while (node) {
                    path.push_back(node->scene);
                    node = node->parent;
                }
                std::reverse(std::begin(path), std::end(path));
                return path;
            }
            auto nbours = current->scene.GetNeighbours();
            for (auto& nbour : nbours) {
                if (hset.count(nbour) == 0) {
                    auto node = std::make_shared<Node>(Node{nbour, current});
                    queue.push_back(node);
                    hset.insert(nbour);
                }
            }
            queue.pop_front();
        }
        return {};
    }

private:
    Field field;
    HSet hset;
};

std::string StringifySolution(const vector<Scene> solution) {
    std::string res = "\"solution\": [\n";
    for (size_t i = 0; i < solution.size(); ++i) {
        const auto& scene = solution.at(i);
        res += "{";
        res += "\"cars\": \n";
        res += StringifyCars(scene.cars);
        res += "\n}";
        if (i + 1 < solution.size()) {
            res += ",";
        }
        res += '\n';
    }
    res += "]";
    return res;
}

std::string StringifyAll(const Field& field, const vector<Scene> solution) {
    std::string res = "{\n";
    res += StringifyField(field) + ",\n";
    res += StringifySolution(solution) + "\n";
    res += "}";
    return res;
}

template<typename T>
T Read(std::istream& istream = std::cin) {
    T value;
    istream >> value;
    return value;
}

Dir ReadDir(std::istream& istream = std::cin) {
    const char sym = Read<char>(istream);
    return sym == 'd' ? Dir::Down : Dir::Right;
}


Field ReadField(std::istream& istream = std::cin) {
    return {
        Read<int>(istream), Read<int>(istream),
        {
            { Read<int>(istream), Read<int>(istream) },
            ReadDir(istream)
        }
    };
}

Car ReadCar(std::istream& istream = std::cin) {
    return {
        Read<int>(istream),
        ReadDir(istream),
        {
            Read<int>(istream),
            Read<int>(istream)
        }
    };
}

vector<Car> ReadCars(size_t count, std::istream& istream = std::cin) {
    vector<Car> cars;
    for (size_t i = 0; i < count; ++i) {
        cars.push_back(ReadCar(istream));
    }
    return cars;
}

int main() {
    const Field field = ReadField();
    const auto cars_count = Read<size_t>();
    const Scene start(field, ReadCars(cars_count));

    Solver solver(field);
    const vector<Scene> solution = solver.Solve(start);
    std::cout << StringifyAll(field, solution) << '\n';
    return 0;
}
