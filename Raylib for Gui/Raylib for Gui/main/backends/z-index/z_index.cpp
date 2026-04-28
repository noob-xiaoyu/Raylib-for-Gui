#include "z_index.h"
#include <algorithm>

// 添加元素
void ZIndexManager::addElement(int id, int z_index, DrawCallback draw) {
    elements[id] = { z_index, std::move(draw) };
    orderDirty = true;
}

// 删除元素
void ZIndexManager::removeElement(int id) {
    elements.erase(id);
    orderDirty = true;
}

// 修改层级
void ZIndexManager::setZIndex(int id, int new_z) {
    auto it = elements.find(id);
    if (it != elements.end()) {
        it->second.z_index = new_z;
        orderDirty = true;
    }
}

// 设置绘制回调
void ZIndexManager::setDrawCallback(int id, DrawCallback draw) {
    auto it = elements.find(id);
    if (it != elements.end()) {
        it->second.draw = std::move(draw);
    }
}

// 置顶：设为当前最高层 + 1
void ZIndexManager::bringToFront(int id) {
    auto it = elements.find(id);
    if (it == elements.end()) return;

    int maxZ = it->second.z_index;
    for (const auto& [eid, data] : elements) {
        if (data.z_index > maxZ) maxZ = data.z_index;
    }
    it->second.z_index = maxZ + 1;
    orderDirty = true;
}

// 置底：设为当前最低层 - 1
void ZIndexManager::sendToBack(int id) {
    auto it = elements.find(id);
    if (it == elements.end()) return;

    int minZ = it->second.z_index;
    for (const auto& [eid, data] : elements) {
        if (data.z_index < minZ) minZ = data.z_index;
    }
    it->second.z_index = minZ - 1;
    orderDirty = true;
}

// 上移一层
void ZIndexManager::moveUp(int id) {
    auto it = elements.find(id);
    if (it != elements.end()) {
        it->second.z_index++;
        orderDirty = true;
    }
}

// 下移一层
void ZIndexManager::moveDown(int id) {
    auto it = elements.find(id);
    if (it != elements.end()) {
        it->second.z_index--;
        orderDirty = true;
    }
}

// 查询层级
int ZIndexManager::getZIndex(int id) const {
    auto it = elements.find(id);
    return (it != elements.end()) ? it->second.z_index : 0;
}

// 检查元素是否存在
bool ZIndexManager::hasElement(int id) const {
    return elements.find(id) != elements.end();
}

// 重建排序缓存（按 z_index 升序）
void ZIndexManager::rebuildOrder() const {
    cachedOrder.clear();
    cachedOrder.reserve(elements.size());
    for (const auto& [id, data] : elements) {
        cachedOrder.push_back(id);
    }
    std::sort(cachedOrder.begin(), cachedOrder.end(),
        [this](int a, int b) {
            return elements.at(a).z_index < elements.at(b).z_index;
        });
    orderDirty = false;
}

// 获取渲染顺序
const std::vector<int>& ZIndexManager::getRenderOrder() const {
    if (orderDirty) rebuildOrder();
    return cachedOrder;
}

// 渲染所有元素
void ZIndexManager::renderAll() const {
    const auto& order = getRenderOrder();
    for (int id : order) {
        auto it = elements.find(id);
        if (it != elements.end() && it->second.draw) {
            it->second.draw(id);
        }
    }
}
