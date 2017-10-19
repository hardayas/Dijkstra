#include "extensions.h"
#include "options.h"

//////////////////////////////////////////////////////////////////////
// For sorting colors

int color_features_compare(const void* vptr_a, const void* vptr_b) {

	const color_features_t* a = (const color_features_t*)vptr_a;
	const color_features_t* b = (const color_features_t*)vptr_b;

	int u = cmp(a->user_index, b->user_index);
	if (u) { return u; }

	int w = cmp(a->wall_dist[0], b->wall_dist[0]);
	if (w) { return w; }

	int g = -cmp(a->wall_dist[1], b->wall_dist[1]);
	if (g) { return g; }

	return -cmp(a->min_dist, b->min_dist);

}

//////////////////////////////////////////////////////////////////////
// Place the game colors into a set order

void game_order_colors(game_info_t* info,
                       game_state_t* state) {

	if (g_options.order_random) {

		srand(now() * 1e6);

		for (size_t i=info->num_colors-1; i>0; --i) {
			size_t j = rand() % (i+1);
			int tmp = info->color_order[i];
			info->color_order[i] = info->color_order[j];
			info->color_order[j] = tmp;
		}

	} else { // not random

		color_features_t cf[MAX_COLORS];
		memset(cf, 0, sizeof(cf));

		for (size_t color=0; color<info->num_colors; ++color) {
			cf[color].index = color;
			cf[color].user_index = MAX_COLORS;
		}


		for (size_t color=0; color<info->num_colors; ++color) {

			int x[2], y[2];

			for (int i=0; i<2; ++i) {
				pos_get_coords(state->pos[color], x+i, y+i);
				cf[color].wall_dist[i] = get_wall_dist(info, x[i], y[i]);
			}

			int dx = abs(x[1]-x[0]);
			int dy = abs(y[1]-y[0]);

			cf[color].min_dist = dx + dy;



		}


		qsort(cf, info->num_colors, sizeof(color_features_t),
		      color_features_compare);

		for (size_t i=0; i<info->num_colors; ++i) {
			info->color_order[i] = cf[i].index;
		}

	}

	if (!g_options.display_quiet) {

		printf("\n************************************************"
		       "\n*               Branching Order                *\n");
		if (g_options.order_most_constrained) {
			printf("* Will choose color by most constrained\n");
		} else {
			printf("* Will choose colors in order: ");
			for (size_t i=0; i<info->num_colors; ++i) {
				int color = info->color_order[i];
				printf("%s", color_name_str(info, color));
			}
			printf("\n");
		}
		printf ("*************************************************\n\n");

	}

}



//////////////////////////////////////////////////////////////////////
// Check for dead-end regions of freespace where there is no way to
// put an active path into and out of it. Any freespace node which
// has only one free neighbor represents such a dead end. For the
// purposes of this check, cur and goal positions count as "free".

int game_check_deadends(const game_info_t* info,
                        const game_state_t* state) {

	size_t colour = state->last_color;

	//gives the current position in the state
	pos_t current_pos = state->pos[colour];

	//check in surrounding positions for dead-ends
	int dir, MAX_DIRS = 4;
	for(dir=0; dir<MAX_DIRS; dir++) {
		pos_t off_pos = pos_offset_pos(info, current_pos, dir);

		if(off_pos != INVALID_POS &&
		    state->cells[off_pos] == 0) {

					//if the off_pos is dead-end, return true
					if(off_pos_deadend(info, state, off_pos)) return 1;

				}
	}

	return 0;

}

//check .h file
int off_pos_deadend(const game_info_t* info, const game_state_t* state,
                    pos_t off_pos) {

	//get the coords for the that position


  int free = 0;

	//for all offsets(pos) 4 dirs
  for (int dir=0; dir<4; ++dir) {

    pos_t neighbor = pos_offset_pos(info, off_pos, dir);
		//if neighbor_pos is inside the game
    if (neighbor != INVALID_POS) {
			//if neighbor_pos cell is free
      if (state->cells[neighbor] == 0) {
        free++;
      } else { // otherwise if the neighbor_pos...
        for (size_t color=0; color < info->num_colors; ++color) {
          if (state->completed & (1 << color)) {
            continue;
          }
					// has the same colour of the current pos
					// or it is the goal pos
          if (neighbor == state->pos[color] ||
              neighbor == info->goal_pos[color]) {
          	free++;
          }
        }
      }
    }
  }

  return free <= 1;

}
