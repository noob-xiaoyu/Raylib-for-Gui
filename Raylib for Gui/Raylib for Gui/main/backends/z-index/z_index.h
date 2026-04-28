#ifndef Z_INDEX_HPP
#define Z_INDEX_HPP

#include <functional>
#include <unordered_map>
#include <vector>

// 绘制回调类型：参数为元素 id，返回 void
using DrawCallback = std::function<void(int id)>;

// 每个可绘制元素的 Z 轴层级信息
struct ZIndexElement {
    int id;           // 元素唯一标识符
    int z_index;      // 层级值（默认 0，越高越靠前）
    DrawCallback draw; // 绘制回调，由上层 GUI 系统设置
};

// Z-Index 管理器
class ZIndexManager {
public:
    // 增
    void addElement(int id, int z_index = 0, DrawCallback draw = nullptr);
    // 删
    void removeElement(int id);
    // 改
    void setZIndex(int id, int new_z);
    // 设置绘制回调
    void setDrawCallback(int id, DrawCallback draw);
    // 快捷操作
    void bringToFront(int id);   // 置顶
    void sendToBack(int id);     // 置底
    void moveUp(int id);         // 上移一层
    void moveDown(int id);       // 下移一层
    // 查
    int  getZIndex(int id) const;
    bool hasElement(int id) const;
    // 按 z-index 升序遍历（低→高，先绘制低层）
    const std::vector<int>& getRenderOrder() const;
    // 渲染所有元素：按 z-index 升序依次调用绘制回调
    void renderAll() const;

private:
    struct ElementData {
        int z_index = 0;
        DrawCallback draw;
    };

    std::unordered_map<int, ElementData> elements; // id → 元素数据
    mutable std::vector<int> cachedOrder;          // 缓存的排序结果
    mutable bool orderDirty = true;                // 排序缓存是否脏了

    void rebuildOrder() const;                     // 重建排序缓存
};

#endif // Z_INDEX_HPP
