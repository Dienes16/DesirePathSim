# DesirePathSim
Simulates desire paths by sending people through a randomly generated village

A random set of streets and houses is generated using nested Voronoi patterns. People spawn at houses and move to other houses using A* pathfinding. Along the way, they might leave streets, walk through the woods or cut corners to get to their destination faster. Over time, this creates "desire paths" which start out as dirt roads, but eventually get paved into actual streets, if enough people use it.

Project uses C++17 and [Raylib](https://www.raylib.com/) for rendering.

# Command line arguments
|Argument|Description|
|--------|-----------|
|`-width=<int>`|Screen width in pixels (default 1920)|
|`-height=<int>`|Screen height in pixels (default 1080)|
|`-target_fps=<int>`|Limit FPS (default 60)|
|`-remove_streets=<1/0>`|Remove streets after world generation and let all paths be desire paths (default 0)|
|`-villager_count=<int>`|How many villagers to spawn (default 1000)|
|`-centroid_count=<int>`|How many centroids to use per Voronoi pattern (default 6)|
|`-subdiv_prob_1=<1-100>`|Probability that a Voronoi shape on level 0 receives a subdivision (default 90)|
|`-subdiv_prob_2=<1-100>`|Probability that a Voronoi shape on level 1 receives a subdivision (default 60)|
|`-minkowski_0=<int>`|Minkowski P value for Voronoi generation on level 0 (default 3)|
|`-minkowski_1=<int>`|Minkowski P value for Voronoi generation on level 1 (default 3)|
|`-minkowski_2=<int>`|Minkowski P value for Voronoi generation on level 2 (default 1)|
|`-place_roundabouts=<1/0>`|Place roundabouts during generation (default 0)|
|`-place_ponds=<1/0>`|Place ponds during generation (default 1)|
|`-place_full_paved_areas=<1/0>`|Place fully paved areas during generation (default 1)|
|`-place_large_trees=<1/0>`|Place large trees during generation (default 1)|
|`-place_small_trees=<1/0>`|Place small trees during generation (default 1)|
|`-pathfinding_threads=<int>`|Number of threads to use for pathfinding (default 4)|
|`-pave_desire_paths=<1/0>`|Pave heavily used desires paths (default 1)|
|`-decay_desire_paths=<1/0>`|Decay underused desire paths (default 1)|
