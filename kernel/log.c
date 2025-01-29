#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// Simple logging that allows concurrent FS system calls.
//
// A log transaction contains the updates of multiple FS system
// calls. The logging system only commits when there are
// no FS system calls active. Thus there is never
// any reasoning required about whether a commit might
// write an uncommitted system call's updates to disk.
//
// A system call should call begin_op()/end_op() to mark
// its start and end. Usually begin_op() just increments
// the count of in-progress FS system calls and returns.
// But if it thinks the log is close to running out, it
// sleeps until the last outstanding end_op() commits.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing block #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged block# before commit.
struct logheader {
  int n;
  int block[LOGSIZE];
};

struct log {
  struct spinlock lock;
  int start;
  int size;
  int outstanding; // how many FS sys calls are executing.
  int committing;  // in commit(), please wait.
  int dev;
  struct logheader lh;
};
struct log log;

void
initlog(int dev, struct superblock *sb)
{
  // if (sizeof(struct logheader) >= BSIZE)
  //   panic("initlog: too big logheader");

  // initlock(&log.lock, "log");
  // log.start = sb->logstart;
  // log.size = sb->nlog;
  // log.dev = dev;
  // recover_from_log();
}

// called at the start of each FS system call.
void
begin_op(void)
{
  // acquire(&log.lock);
  // while(1){
  //   if(log.committing){
  //     sleep(&log, &log.lock);
  //   } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
  //     // this op might exhaust log space; wait for commit.
  //     sleep(&log, &log.lock);
  //   } else {
  //     log.outstanding += 1;
  //     release(&log.lock);
  //     break;
  //   }
  // }
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(void)
{
  // int do_commit = 0;

  // acquire(&log.lock);
  // log.outstanding -= 1;
  // if(log.committing)
  //   panic("log.committing");
  // if(log.outstanding == 0){
  //   do_commit = 1;
  //   log.committing = 1;
  // } else {
  //   // begin_op() may be waiting for log space,
  //   // and decrementing log.outstanding has decreased
  //   // the amount of reserved space.
  //   wakeup(&log);
  // }
  // release(&log.lock);

  // if(do_commit){
  //   // call commit w/o holding locks, since not allowed
  //   // to sleep with locks.
  //   commit();
  //   acquire(&log.lock);
  //   log.committing = 0;
  //   wakeup(&log);
  //   release(&log.lock);
  // }
}

// Caller has modified b->data and is done with the buffer.
// Record the block number and pin in the cache by increasing refcnt.
// commit()/write_log() will do the disk write.
//
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  // int i;

  // acquire(&log.lock);
  // if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
  //   panic("too big a transaction");
  // if (log.outstanding < 1)
  //   panic("log_write outside of trans");

  // for (i = 0; i < log.lh.n; i++) {
  //   if (log.lh.block[i] == b->blockno)   // log absorption
  //     break;
  // }
  // log.lh.block[i] = b->blockno;
  // if (i == log.lh.n) {  // Add new block to log?
  //   bpin(b);
  //   log.lh.n++;
  // }
  // release(&log.lock);
}

