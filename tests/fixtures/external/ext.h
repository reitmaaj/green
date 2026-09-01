#ifndef GREEN_FIXTURE_EXT_H
#define GREEN_FIXTURE_EXT_H

/* A deliberately non-owned header. It is reached from fixtures via a
 * relative include that falls outside the primary translation unit's
 * directory, so it must not be treated as project-owned. */

#define EXT_AND(a, b) ((a) && (b))
#define EXT_PURE 42

#endif
