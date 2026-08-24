/* sprite_groups.h — Sprite Groups & Hierarchies for MEGA65
 *
 * Provides sprite grouping with parent-child relationships and hierarchical transforms.
 * Extends Phase 34-36 sprites with organizational and transform composition features.
 *
 * Features:
 * - Sprite groups (containers with multiple sprites)
 * - Parent-child relationships with automatic transform propagation
 * - Hierarchical transforms (parent → child composition)
 * - Group visibility and layer control
 * - Efficient batch operations (move, scale, rotate entire group)
 * - Transform inheritance with local/world space distinction
 *
 * Usage:
 *   #include <sprite_groups.h>
 *   sprite_group_t group = sprite_group_create();
 *   sprite_t child1 = sprite_create(0, 0, 32, 32);
 *   sprite_t child2 = sprite_create(40, 0, 32, 32);
 *   sprite_group_add_sprite(group, child1);
 *   sprite_group_add_sprite(group, child2);
 *   sprite_group_set_position(group, 100, 100);  // Move both children
 *   sprite_group_rotate(group, 45.0f);            // Rotate both around group origin
 */

#ifndef SPRITE_GROUPS_H
#define SPRITE_GROUPS_H

#include <sprites.h>

/* ============================================================================
 * GROUP TYPE DEFINITION
 * ========================================================================== */

typedef void *sprite_group_t;  /* Opaque sprite group handle */

#define INVALID_GROUP  NULL

/* ============================================================================
 * GROUP INFORMATION STRUCTURE
 * ========================================================================== */

typedef struct {
    sprite_group_t id;          /* Group handle */
    int x, y;                   /* Group position (origin point) */
    int visible;                /* Group visibility flag (0-1) */
    int layer;                  /* Base layer for group */
    int member_count;           /* Number of sprites in group */
    int is_transform_group;     /* 1 if group uses transform composition */
} sprite_group_info_t;

/* ============================================================================
 * GROUP CREATION & DESTRUCTION
 * ========================================================================== */

/**
 * sprite_group_create - Create a new sprite group
 *
 * Creates an empty group that can contain multiple sprites.
 * Group has its own position and can have transforms applied.
 *
 * Parameters:
 *   x, y — Initial group position (origin point)
 *
 * Returns:
 *   Group handle on success, INVALID_GROUP on error
 */
sprite_group_t sprite_group_create(int x, int y);

/**
 * sprite_group_destroy - Delete a sprite group
 *
 * Removes group from display. Does NOT destroy sprites in group —
 * they remain valid and can be added to other groups.
 *
 * Parameters:
 *   group — Group handle to destroy
 */
void sprite_group_destroy(sprite_group_t group);

/**
 * sprite_group_get_info - Get group information
 *
 * Reads current group properties.
 *
 * Parameters:
 *   group — Group handle
 *   info — Pointer to sprite_group_info_t to receive data
 *
 * Returns:
 *   1 if successful, 0 if group invalid
 */
int sprite_group_get_info(sprite_group_t group, sprite_group_info_t *info);

/* ============================================================================
 * GROUP MEMBERSHIP
 * ========================================================================== */

/**
 * sprite_group_add_sprite - Add sprite to group
 *
 * Adds existing sprite to group. Sprite can belong to only one group.
 * Adding sprite to different group removes it from previous group.
 *
 * Parameters:
 *   group — Group handle
 *   sprite — Sprite to add
 *
 * Returns:
 *   1 if successful, 0 if group/sprite invalid
 */
int sprite_group_add_sprite(sprite_group_t group, sprite_t sprite);

/**
 * sprite_group_remove_sprite - Remove sprite from group
 *
 * Removes sprite from group. Sprite remains valid and visible.
 *
 * Parameters:
 *   group — Group handle
 *   sprite — Sprite to remove
 *
 * Returns:
 *   1 if successful, 0 if not in group
 */
int sprite_group_remove_sprite(sprite_group_t group, sprite_t sprite);

/**
 * sprite_group_clear - Remove all sprites from group
 *
 * Parameters:
 *   group — Group handle
 *
 * Returns:
 *   Number of sprites removed
 */
int sprite_group_clear(sprite_group_t group);

/**
 * sprite_group_get_member_count - Get number of sprites in group
 *
 * Parameters:
 *   group — Group handle
 *
 * Returns:
 *   Number of sprites (0 if empty or invalid)
 */
int sprite_group_get_member_count(sprite_group_t group);

/**
 * sprite_group_get_member - Get sprite by index
 *
 * Parameters:
 *   group — Group handle
 *   index — Member index (0 to count-1)
 *
 * Returns:
 *   Sprite handle, or INVALID_SPRITE if index out of range
 */
sprite_t sprite_group_get_member(sprite_group_t group, int index);

/**
 * sprite_is_in_group - Check if sprite belongs to group
 *
 * Parameters:
 *   group — Group handle
 *   sprite — Sprite to check
 *
 * Returns:
 *   1 if sprite in group, 0 otherwise
 */
int sprite_is_in_group(sprite_group_t group, sprite_t sprite);

/* ============================================================================
 * GROUP POSITIONING
 * ========================================================================== */

/**
 * sprite_group_set_position - Move group to new position
 *
 * Moves group origin point. All member sprites move relative to group.
 *
 * Parameters:
 *   group — Group handle
 *   x, y — New position (pixels)
 */
void sprite_group_set_position(sprite_group_t group, int x, int y);

/**
 * sprite_group_move - Move group by relative offset
 *
 * Parameters:
 *   group — Group handle
 *   dx, dy — Relative movement (pixels)
 */
void sprite_group_move(sprite_group_t group, int dx, int dy);

/**
 * sprite_group_get_position - Get group position
 *
 * Parameters:
 *   group — Group handle
 *   x, y — Pointers to receive position
 *
 * Returns:
 *   1 if successful, 0 if group invalid
 */
int sprite_group_get_position(sprite_group_t group, int *x, int *y);

/**
 * sprite_group_get_bounds - Get group bounding box
 *
 * Returns axis-aligned bounding box containing all member sprites.
 *
 * Parameters:
 *   group — Group handle
 *   x1, y1 — Pointers for top-left corner
 *   x2, y2 — Pointers for bottom-right corner
 *
 * Returns:
 *   1 if successful, 0 if group empty/invalid
 */
int sprite_group_get_bounds(sprite_group_t group, int *x1, int *y1,
                            int *x2, int *y2);

/* ============================================================================
 * GROUP VISIBILITY & ATTRIBUTES
 * ========================================================================== */

/**
 * sprite_group_set_visible - Show or hide entire group
 *
 * Affects visibility of all member sprites.
 * Individual sprite visibility can override group visibility.
 *
 * Parameters:
 *   group — Group handle
 *   visible — 1 to show, 0 to hide
 */
void sprite_group_set_visible(sprite_group_t group, int visible);

/**
 * sprite_group_is_visible - Check if group is visible
 *
 * Returns:
 *   1 if visible, 0 if hidden
 */
int sprite_group_is_visible(sprite_group_t group);

/**
 * sprite_group_set_layer - Set base layer for group
 *
 * All member sprites are offset by this base layer value.
 * Allows reordering entire group relative to other sprites.
 *
 * Parameters:
 *   group — Group handle
 *   layer — Base layer index (0-255)
 */
void sprite_group_set_layer(sprite_group_t group, int layer);

/**
 * sprite_group_get_layer - Get group layer
 *
 * Returns:
 *   Layer index (0-255)
 */
int sprite_group_get_layer(sprite_group_t group);

/* ============================================================================
 * GROUP TRANSFORMS
 * ========================================================================== */

/**
 * sprite_group_scale - Scale all sprites in group
 *
 * Applies scaling transform to group. Children scale around group origin.
 *
 * Parameters:
 *   group — Group handle
 *   scale_x, scale_y — Scale factors (1.0 = normal)
 */
void sprite_group_scale(sprite_group_t group, float scale_x, float scale_y);

/**
 * sprite_group_scale_uniform - Uniformly scale group
 *
 * Parameters:
 *   group — Group handle
 *   scale — Scale factor (1.0 = normal)
 */
void sprite_group_scale_uniform(sprite_group_t group, float scale);

/**
 * sprite_group_get_scale - Get group scale
 *
 * Parameters:
 *   group — Group handle
 *   scale_x, scale_y — Pointers to receive scale factors
 *
 * Returns:
 *   1 if successful, 0 if group invalid
 */
int sprite_group_get_scale(sprite_group_t group, float *scale_x,
                           float *scale_y);

/**
 * sprite_group_rotate - Rotate all sprites in group
 *
 * Applies rotation transform to group. Children rotate around group origin.
 *
 * Parameters:
 *   group — Group handle
 *   angle_degrees — Rotation angle (0-360 degrees)
 */
void sprite_group_rotate(sprite_group_t group, float angle_degrees);

/**
 * sprite_group_get_rotation - Get group rotation
 *
 * Returns:
 *   Rotation angle in degrees (0-360)
 */
float sprite_group_get_rotation(sprite_group_t group);

/**
 * sprite_group_rotate_relative - Rotate by relative amount
 *
 * Parameters:
 *   group — Group handle
 *   delta_degrees — Relative rotation
 */
void sprite_group_rotate_relative(sprite_group_t group, float delta_degrees);

/**
 * sprite_group_reset_transform - Reset group transform to identity
 *
 * Parameters:
 *   group — Group handle
 */
void sprite_group_reset_transform(sprite_group_t group);

/**
 * sprite_group_set_transform_origin - Set group transform origin
 *
 * Sets the point around which group scales and rotates.
 *
 * Parameters:
 *   group — Group handle
 *   x, y — Origin point (relative to group position)
 */
void sprite_group_set_transform_origin(sprite_group_t group, int x, int y);

/**
 * sprite_group_get_transform_origin - Get group transform origin
 *
 * Parameters:
 *   group — Group handle
 *   x, y — Pointers to receive origin
 */
void sprite_group_get_transform_origin(sprite_group_t group, int *x, int *y);

/* ============================================================================
 * PARENT-CHILD RELATIONSHIPS
 * ========================================================================== */

/**
 * sprite_set_parent - Set sprite's parent sprite
 *
 * Creates parent-child relationship. Child inherits parent's transform.
 * Only one parent per sprite; setting new parent removes old relationship.
 *
 * Parameters:
 *   sprite — Child sprite
 *   parent — Parent sprite (or NULL to remove parent)
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_set_parent(sprite_t sprite, sprite_t parent);

/**
 * sprite_get_parent - Get sprite's parent
 *
 * Parameters:
 *   sprite — Child sprite
 *
 * Returns:
 *   Parent sprite handle, or INVALID_SPRITE if no parent
 */
sprite_t sprite_get_parent(sprite_t sprite);

/**
 * sprite_get_child_count - Get number of children
 *
 * Parameters:
 *   sprite — Parent sprite
 *
 * Returns:
 *   Number of direct children
 */
int sprite_get_child_count(sprite_t sprite);

/**
 * sprite_get_child - Get child by index
 *
 * Parameters:
 *   sprite — Parent sprite
 *   index — Child index (0 to count-1)
 *
 * Returns:
 *   Child sprite handle, or INVALID_SPRITE if index out of range
 */
sprite_t sprite_get_child(sprite_t sprite, int index);

/**
 * sprite_is_child_of - Check parent-child relationship
 *
 * Parameters:
 *   child — Potential child sprite
 *   parent — Potential parent sprite
 *
 * Returns:
 *   1 if child is direct child of parent, 0 otherwise
 */
int sprite_is_child_of(sprite_t child, sprite_t parent);

/**
 * sprite_is_ancestor_of - Check if ancestor in hierarchy
 *
 * Recursively checks if ancestor appears above sprite in hierarchy.
 *
 * Parameters:
 *   ancestor — Potential ancestor sprite
 *   descendant — Potential descendant sprite
 *
 * Returns:
 *   1 if ancestor is in descendant's parent chain, 0 otherwise
 */
int sprite_is_ancestor_of(sprite_t ancestor, sprite_t descendant);

/* ============================================================================
 * BATCH OPERATIONS
 * ========================================================================== */

/**
 * sprite_group_update_all - Update all sprites in group
 *
 * Updates movement and animation for all member sprites.
 * Respects parent-child relationships and hierarchical transforms.
 *
 * Parameters:
 *   group — Group handle
 *   delta_ms — Elapsed time in milliseconds
 *
 * Returns:
 *   Number of sprites updated
 */
int sprite_group_update_all(sprite_group_t group, int delta_ms);

/**
 * sprite_group_render_all - Render entire group
 *
 * Renders all member sprites respecting group transforms,
 * visibility, and Z-order.
 *
 * Parameters:
 *   group — Group handle
 *
 * Returns:
 *   Number of sprites rendered
 */
int sprite_group_render_all(sprite_group_t group);

/**
 * sprite_group_collides_point - Test point collision with any member
 *
 * Tests if point collides with any sprite in group.
 *
 * Parameters:
 *   group — Group handle
 *   x, y — Screen coordinates
 *
 * Returns:
 *   1 if point collides with any member, 0 otherwise
 */
int sprite_group_collides_point(sprite_group_t group, int x, int y);

/**
 * sprite_group_collides_point_get - Find sprite at point
 *
 * Finds topmost sprite in group at given coordinates.
 *
 * Parameters:
 *   group — Group handle
 *   x, y — Screen coordinates
 *
 * Returns:
 *   Sprite handle of topmost sprite at point, INVALID_SPRITE if none
 */
sprite_t sprite_group_collides_point_get(sprite_group_t group, int x, int y);

/* ============================================================================
 * GROUP ENUMERATION
 * ========================================================================== */

/**
 * sprite_group_enumerate - Enumerate all groups
 *
 * Calls callback for each active sprite group.
 *
 * Parameters:
 *   callback — Function to call for each group (or NULL to count only)
 *
 * Returns:
 *   Total number of groups
 */
typedef void (*sprite_group_callback)(sprite_group_t group);
int sprite_group_enumerate(sprite_group_callback callback);

/**
 * sprite_count_all_groups - Get total number of groups
 *
 * Returns:
 *   Count of active sprite groups
 */
int sprite_count_all_groups(void);

/* ============================================================================
 * DEBUGGING & INSPECTION
 * ========================================================================== */

/**
 * sprite_print_group - Print group information (debug)
 *
 * Parameters:
 *   group — Group handle
 */
void sprite_print_group(sprite_group_t group);

/**
 * sprite_print_hierarchy - Print sprite hierarchy (debug)
 *
 * Parameters:
 *   sprite — Root sprite to print hierarchy from
 *   indent — Initial indentation level (typically 0)
 */
void sprite_print_hierarchy(sprite_t sprite, int indent);

#endif
