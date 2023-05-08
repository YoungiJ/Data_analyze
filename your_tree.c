#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DIMENSIONS 2
#define MAX_ENTRIES 8

struct Rect {
    double min[DIMENSIONS];
    double max[DIMENSIONS];
    int x1, y1, x2, y2;

};

struct Node {
    int is_leaf; /* 1 if the node is a leaf, 0 otherwise */
    int num_entries;
    struct Rect mbr; /* minimum bounding rectangle */
    struct Node** children;
    struct Entry* entries;
    struct Node* parent;
};


    struct Entry {
        struct Rect mbr;
        struct Node* child;
    };

    struct RTree {
        struct Node* root;
    };

    double min(double a, double b) {
        return a < b ? a : b;
    }

    double max(double a, double b) {
        return a > b ? a : b;
    }

    struct Rect rect(int x1, int y1, int x2, int y2) {
        struct Rect r = { x1, y1, x2, y2 };
        return r;
    }

    double rect_area(struct Rect r) {
        double area = 1;
        for (int i = 0; i < DIMENSIONS; i++) {
            area *= r.max[i] - r.min[i];
        }
        return area;
    }

    double rect_distance(struct Rect a, struct Rect b) {
        double distance = 0;
        for (int i = 0; i < DIMENSIONS; i++) {
            if (a.max[i] < b.min[i]) {
                distance += pow(b.min[i] - a.max[i], 2);
            }
            else if (a.min[i] > b.max[i]) {
                distance += pow(a.min[i] - b.max[i], 2);
            }
        }
        return sqrt(distance);
    }

    struct Rect rect_combine(struct Rect a, struct Rect b) {
        struct Rect result;
        for (int i = 0; i < DIMENSIONS; i++) {
            result.min[i] = min(a.min[i], b.min[i]);
            result.max[i] = max(a.max[i], b.max[i]);
        }
        return result;
    }

    void node_init(struct Node* node, int is_leaf) {
        node->is_leaf = is_leaf;
        node->num_entries = 0;
        node->children = (struct Node**)malloc(MAX_ENTRIES * sizeof(struct Node*));
    }

    void node_add_entry(struct Node* node, struct Rect r, struct Node* child) {

        node->children[node->num_entries] = child;
        node->mbr = node->num_entries == 0 ? r : rect_combine(node->mbr, r);
        node->num_entries++;
    }

    struct Node* node_choose_subtree(struct Node* node, struct Rect r) {
        double min_distance = -1;
        struct Node* result = NULL;
        for (int i = 0; i < node->num_entries; i++) {
            double distance = rect_distance(node->children[i]->mbr, r);
            if (min_distance == -1 || distance < min_distance) {
                min_distance = distance;
                result = node->children[i];
            }
        }
        return result;
    }

    void node_split(struct Node* node, struct Node** new_node) {
        int split_axis = 0;
        double max_width = 0;
        for (int i = 0; i < DIMENSIONS; i++) {
            double width = node->mbr.max[i] - node->mbr.min[i];
            if (width > max_width) {
                max_width = width;
                split_axis = i;
            }
        }

        int split_index = node->num_entries / 2;
        *new_node = (struct Node*)malloc(sizeof(struct Node));
        node_init(*new_node, node->is_leaf);

        if (node->is_leaf) {
            /* Split leaf node */
            for (int i = split_index; i < node->num_entries; i++) {
                node->children[i]->mbr = rect_combine(node->children[i]->mbr, (*new_node)->mbr);
                node_add_entry(*new_node, node->children[i]->mbr, node->children[i]);
            }
            node->num_entries = split_index;
            (*new_node)->mbr = node->children[split_index]->mbr;
        }
        else {
            /* Split non-leaf node */
            for (int i = split_index; i < node->num_entries; i++) {
                node_add_entry(*new_node, node->children[i]->mbr, node->children[i]);
                node->children[i] = NULL;
            }
            node->num_entries = split_index;
            (*new_node)->mbr = (*new_node)->children[0]->mbr;
        }
    }

    void rtree_insert_rec(struct Node* node, struct Rect r, struct Node* child, struct Node** new_node) {
        if (node->is_leaf && node->num_entries < MAX_ENTRIES) {
            node_add_entry(node, r, child);
            return;
        }
        struct Node* sub_tree = NULL;
        if (!node->is_leaf) {
            sub_tree = node_choose_subtree(node, r);
            rtree_insert_rec(sub_tree, r, child, new_node);
        }

        if (*new_node) {
            node_add_entry(node, (*new_node)->mbr, *new_node);
            *new_node = NULL;
        }
        else if (node->is_leaf && node->num_entries == MAX_ENTRIES) {
            /* Split leaf node */
            node_split(node, new_node);
            if (node == sub_tree) {
                sub_tree = *new_node;
            }
            node_add_entry(sub_tree, r, child);
        }
        else if (!node->is_leaf && node->num_entries == MAX_ENTRIES) {
            /* Split non-leaf node */
            node_split(node, new_node);
            if (node == sub_tree) {
                sub_tree = *new_node;
            }
            rtree_insert_rec(sub_tree, r, child, new_node);
        }
    }

    void rtree_insert(struct RTree* tree, struct Rect r) {
        struct Node* new_node = NULL;
        struct Node* child = (struct Node*)malloc(sizeof(struct Node));
        node_init(child, 1);
        child->mbr = r;
        rtree_insert_rec(tree->root, r, child, &new_node);
        if (new_node) {
            // Grow tree taller and new root 
            struct Node* old_root = tree->root;
            tree->root = (struct Node*)malloc(sizeof(struct Node));
            node_init(tree->root, 0);
            node_add_entry(tree->root, old_root->mbr, old_root);
            node_add_entry(tree->root, new_node->mbr,


                new_node);
        }
    }

    int rect_intersect(struct Rect a, struct Rect b) {
        if (a.x1 > b.x2 || a.x2 < b.x1) return 0;
        if (a.y1 > b.y2 || a.y2 < b.y1) return 0;
        return 1;
    }
    


    void node_search(struct Node* node, struct Rect r, int* count, void (*callback)(void* data)) {
        if (node->is_leaf) {
            for (int i = 0; i < node->num_entries; i++) {
                if (rect_intersect(node->children[i]->mbr, r)) {
                    (*count)++;
                    callback(node->children[i]);
                }
            }
        }
        else {
            for (int i = 0; i < node->num_entries; i++) {
                if (rect_intersect(node->children[i]->mbr, r)) {
                    node_search(node->children[i], r, count, callback);
                }
            }
        }
    }



    void rtree_search(struct RTree* tree, struct Rect r, void (callback)(void* data)) {
        int count = 0;
        node_search(tree->root, r, &count, callback);
    }

    void print_rect(void* data) {
        struct Rect* rect = (struct Rect*)data;
        printf("(%d, %d, %d, %d)\n", rect->x1, rect->y1, rect->x2, rect->y2);
    }

    void rtree_init(struct RTree* tree) {
        tree->root = (struct Node*)malloc(sizeof(struct Node));
        tree->root->is_leaf = 1;
        tree->root->num_entries = 0;
        if (tree->root->is_leaf) {
            tree->root->entries = (struct Entry*)malloc(MAX_ENTRIES * sizeof(struct Entry));
        }
        else {
            tree->root->children = (struct Node**)malloc(MAX_ENTRIES * sizeof(struct Node*));
        }
    }

    void node_clear(struct Node* node) {
        if (node->is_leaf) {
            for (int i = 0; i < node->num_entries; i++) {
                free(node->children[node->num_entries]);
            }
        }
        else {
            for (int i = 0; i < node->num_entries; i++) {
                node_clear(node->children[node->num_entries]);
                free(node->children[node->num_entries]);
            }
        }
        free(node->entries);
        free(node);
    }

    void rtree_clear(struct RTree* tree) {
        node_clear(tree->root);
    }


    int main() {
        struct RTree tree;
        rtree_init(&tree);

        /* Insert some rectangles */
        rtree_insert(&tree, rect(0, 0, 2, 2));
        rtree_insert(&tree, rect(2, 2, 4, 4));
        rtree_insert(&tree, rect(3, 3, 5, 5));
        rtree_insert(&tree, rect(1, 1, 3, 3));

        /* Search for rectangles intersecting with the query rectangle (1, 1, 4, 4) */
        printf("Rectangles intersecting with (1, 1, 4, 4):\n");
        rtree_search(&tree, rect(1, 1, 4, 4), print_rect);

        /* Clean up */
        rtree_clear(&tree);
        return 0;
    }
