/*
 * HeapUsage.h
 *
 *  Created on: 2014-12-04
 *      Author: Roger
 */

#ifndef HEAPUSAGE_H_
#define HEAPUSAGE_H_

/*
 *
 * #include <src/Logger/HeapUsage/HeapUsage.h>
 *
 */

#include <QObject>

#include <sys/procfs.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>

using namespace std;

class HeapUsage : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE static double measureMem() {
        // This is BlackBerry Shadid Haque code to retrieve the heap used by an headless app.
        // you can call this functions anytime to see if it gets over the 3Mb limit.
           int rc, n, i, num;
           int64_t memsize = -1ll;
           procfs_mapinfo *maps = NULL;
           int fd = open("/proc/self/as", O_RDONLY);
           if (fd == -1)
                  return -1ll;
           do {
                  rc = devctl(fd, DCMD_PROC_PAGEDATA, NULL, 0, &num);
                  if (rc)
                         goto cleanup;
                  maps = (procfs_mapinfo*) realloc(maps, num * sizeof(*maps));
                  if (!maps)
                         goto cleanup;
                  // pre-subtract the allocation we just made, rounded up to page size
                  memsize = 0ll - ((num * sizeof(*maps) + __PAGESIZE - 1) & ~(__PAGESIZE - 1));
                  rc = devctl(fd, DCMD_PROC_PAGEDATA, maps, num * sizeof(*maps), &n);
                  if (rc) {
                         memsize = -1ll;
                         goto cleanup;
                  }
           } while (n > num);
           for (i = 0; i < n; i++) {
                  if ((maps[i].flags & (MAP_ANON | MAP_STACK | MAP_ELF | MAP_TYPE)) == (MAP_ANON | MAP_PRIVATE)) {
                         memsize += maps[i].size;
                  }
           }

           cleanup: close(fd);
           free(maps);

           double toReturn = memsize / 10000.0;
           return toReturn;
    }
};

#endif /* HEAPUSAGE_H_ */
