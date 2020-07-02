/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2019, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* <DESC>
 * Download many files in parallel, in the same thread.
 * </DESC>
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#include <errno.h>

#define MAX_PARALLEL 100

typedef struct membuf_type {
  char *mem;
  size_t len;
  size_t size;
} membuf_t;

int read_user(FILE *fp, char *user, size_t sz) {
  int n;
  char *chp;

  if (feof(fp)) { return 0; }
  chp = fgets(user, 31, fp);

  if (!chp) { return 0; }
  user[sz-1] = '\0';
  n = strlen(user);
  if ((n>0) && (user[n-1]=='\n')) {
    user[n-1] = '\0';
    n--;
  }
  return n;
}

static size_t write_cb(char *data, size_t sz, size_t nm, void *userp) {
  int i;
  char *ptr;
  size_t rsz = sz * nm;
  membuf_t *mem = (membuf_t *)userp;

  if ( (mem->len + (sz*nm)) >= (mem->size) ) {
    ptr = (char *)malloc(sizeof(char)*(mem->len + (sz*nm) + 1));
    memcpy(ptr, mem->mem, mem->len);
    mem->mem = ptr;
    mem->size = mem->len + (sz*nm) + 1;
    mem->mem[mem->size-1] = '\0';
  }

  memcpy( &(mem->mem[mem->len]), data, sz*nm);
  mem->len += sz*nm;
  mem->mem[mem->len] = '\0';

  return sz*nm;
}

static void add_transfer(CURLM *cm, char *url) {
  void *v;
  membuf_t *mbuf = (membuf_t *)malloc(sizeof(membuf_t));

  mbuf->len = 0;
  mbuf->size = 1024*1024;
  mbuf->mem = (char *)malloc(sizeof(char)*(mbuf->size));
  v = (void *)mbuf;

  CURL *eh = curl_easy_init();
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(eh, CURLOPT_WRITEDATA, v);
  curl_easy_setopt(eh, CURLOPT_URL, url);
  curl_easy_setopt(eh, CURLOPT_PRIVATE, v);
  curl_multi_add_handle(cm, eh);
}

int main(int argc, char **argv) {
  CURLM *cm;
  CURLMsg *msg;
  unsigned int transfers = 0;
  int msgs_left = -1;
  int still_alive = 1;

  int n;
  int user_file_eof=0;

  int n_url = 30;
  int item_id = 1001;
  char *url = NULL;
  int i;
  int requests=0;

  membuf_t *mbuf;

  FILE *fp=NULL;
  char user[32], *chp;

  if (argc != 2) {
    fprintf(stderr, "usage: hnau <user-file>\n");
    exit(-1);
  }

  if ((fp=fopen(argv[1], "r"))==NULL) {
    perror(argv[1]);
    exit(-2);
  }

  curl_global_init(CURL_GLOBAL_ALL);
  cm = curl_multi_init();

  /* Limit the amount of simultaneous connections curl should allow: */
  curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long)MAX_PARALLEL);

  url = (char *)malloc(sizeof(char)*1024);

  for (transfers = 0; transfers < MAX_PARALLEL; transfers++) {
    read_user(fp, user, 32);
    memset(url, 0, sizeof(char)*1024);
    sprintf(url, "https://hacker-news.firebaseio.com/v0/user/%s.json", user);
    add_transfer(cm, url);
    requests++;
    continue;
  }


  do {

    if ((requests==0) && (user_file_eof)) {
      break;
    }

    curl_multi_perform(cm, &still_alive);

    while((msg = curl_multi_info_read(cm, &msgs_left))) {
      if(msg->msg == CURLMSG_DONE) {
        CURL *e = msg->easy_handle;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &mbuf);

        printf("%s\n", mbuf->mem);
        free(mbuf->mem);
        free(mbuf);

        curl_multi_remove_handle(cm, e);
        curl_easy_cleanup(e);

        requests--;
      }
      else {
        fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
      }

      if (read_user(fp, user, 32)) {
        memset(url, 0, sizeof(char)*1024);
        sprintf(url, "https://hacker-news.firebaseio.com/v0/user/%s.json", user);
        transfers++;

        add_transfer(cm, url);
        requests++;
      }
      else {
        user_file_eof = 1;
      }
    }
    if (still_alive) {
      curl_multi_wait(cm, NULL, 0, 1000, NULL);
    }

  //} while(still_alive || (transfers < n_url));
  } while(still_alive);

  curl_multi_cleanup(cm);
  curl_global_cleanup();

  fclose(fp);

  return EXIT_SUCCESS;
}
