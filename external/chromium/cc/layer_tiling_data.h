// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef CC_LAYER_TILING_DATA_H_
#define CC_LAYER_TILING_DATA_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "cc/cc_export.h"
#include "cc/hash_pair.h"
#include "cc/region.h"
#include "cc/scoped_ptr_hash_map.h"
#include "cc/tiling_data.h"
#include "ui/gfx/rect.h"

#if defined(__LB_USE_SHELL_REUSABLE_ALLOCATOR__)
#include "cc/shell_allocator.h"
#endif

namespace cc {

class CC_EXPORT LayerTilingData {
public:
    enum BorderTexelOption { HasBorderTexels, NoBorderTexels };

    ~LayerTilingData();

    static scoped_ptr<LayerTilingData> create(const gfx::Size& tileSize, BorderTexelOption);

    bool hasEmptyBounds() const { return m_tilingData.has_empty_bounds(); }
    int numTilesX() const { return m_tilingData.num_tiles_x(); }
    int numTilesY() const { return m_tilingData.num_tiles_y(); }
    gfx::Rect tileBounds(int i, int j) const { return m_tilingData.TileBounds(i, j); }
    gfx::Vector2d textureOffset(int xIndex, int yIndex) const { return m_tilingData.TextureOffset(xIndex, yIndex); }

    // Change the tile size. This may invalidate all the existing tiles.
    void setTileSize(const gfx::Size&);
    gfx::Size tileSize() const;
    // Change the border texel setting. This may invalidate all existing tiles.
    void setBorderTexelOption(BorderTexelOption);
    bool hasBorderTexels() const { return !!m_tilingData.border_texels(); }

    bool isEmpty() const { return hasEmptyBounds() || !tiles().size(); }

    const LayerTilingData& operator=(const LayerTilingData&);

    class Tile {
    public:
        Tile() : m_i(-1), m_j(-1) { }
        virtual ~Tile() { }

        int i() const { return m_i; }
        int j() const { return m_j; }
        void moveTo(int i, int j) { m_i = i; m_j = j; }

        const gfx::Rect& opaqueRect() const { return m_opaqueRect; }
        void setOpaqueRect(const gfx::Rect& opaqueRect) { m_opaqueRect = opaqueRect; }
    private:
        int m_i;
        int m_j;
        gfx::Rect m_opaqueRect;
        DISALLOW_COPY_AND_ASSIGN(Tile);
    };
    typedef std::pair<int, int> TileMapKey;
#if defined(__LB_USE_SHELL_REUSABLE_ALLOCATOR__)
    typedef base::shell_hash_map_node_sim<std::pair<const TileMapKey, Tile*> > LayerTilingDataItemType;
    static const int kMaxLayerTilingDataReuseListCount = 24;
    // In my test run the max count of node object was around 128.
    // I set it to 160 here. Each node is 20 bytes so it is about 3.2k
    // preserved memory here.
    static const int kPreservedNumberOfItems = 160;
    class LayerTilingDataReuseList :
          public base::ShellAllocatorBufferList<kMaxLayerTilingDataReuseListCount> {
    public:
        LayerTilingDataReuseList() {
          // Init the preserved buffer for user to use without allocations.
          // The object is a static object, hence the buffer should be
          // relatively at beginning of heap (improve memory fragmentation
          // issue).
          init_list(0, kNodeSize, kPreservedNumberOfItems, preserved_buffer_);
        }

        static LayerTilingDataReuseList* get_instance();

    private:
        static const int kNodeSize = sizeof(LayerTilingDataItemType);

        char preserved_buffer_[kPreservedNumberOfItems][kNodeSize];
    };
    typedef ScopedPtrHashMap<TileMapKey, Tile, base::ShellAllocator<LayerTilingDataItemType, LayerTilingDataReuseList> > TileMap;
#else
    typedef ScopedPtrHashMap<TileMapKey, Tile> TileMap;
#endif

    void addTile(scoped_ptr<Tile>, int, int);
    scoped_ptr<Tile> takeTile(int, int);
    Tile* tileAt(int, int) const;
    const TileMap& tiles() const { return m_tiles; }

    void setBounds(const gfx::Size&);
    gfx::Size bounds() const;

    void contentRectToTileIndices(const gfx::Rect&, int &left, int &top, int &right, int &bottom) const;
    gfx::Rect tileRect(const Tile*) const;

    Region opaqueRegionInContentRect(const gfx::Rect&) const;

    void reset();

protected:
    LayerTilingData(const gfx::Size& tileSize, BorderTexelOption);

    TileMap m_tiles;
    TilingData m_tilingData;
};

}

#endif  // CC_LAYER_TILING_DATA_H_
