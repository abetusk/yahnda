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

#define MAX_PARALLEL 100

typedef struct membuf_type {
  char *mem;
  size_t sz;
} membuf_t;

static size_t write_cb(char *data, size_t sz, size_t nm, void *userp) {
  int i;
  //size_t rsz = sz * nm;
  //membuf_t *mem = (membuf_t *)userp;
  //char *ptr = (char *)malloc(

  for (i=0; i<(sz*nm); i++) {
    printf("%c", data[i]);
  }
  printf("\n");

  return sz*nm;
}

static void add_transfer(CURLM *cm, int i, char *url) {
  CURL *eh = curl_easy_init();
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(eh, CURLOPT_URL, url);
  curl_easy_setopt(eh, CURLOPT_PRIVATE, url);
  curl_multi_add_handle(cm, eh);
}

int main(int argc, char **argv) {
  CURLM *cm;
  CURLMsg *msg;
  unsigned int transfers = 0;
  int msgs_left = -1;
  int still_alive = 1;

  int n_url = 30;
  int item_id = 1001;
  char *url = NULL;

  if (argc!=3) {
    fprintf(stderr, "usage: hn-parallel <item-start> <n-item>\n");
    exit(-1);
  }

  item_id = atoi(argv[1]);
  n_url = atoi(argv[2]);

  curl_global_init(CURL_GLOBAL_ALL);
  cm = curl_multi_init();

  /* Limit the amount of simultaneous connections curl should allow: */
  curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long)MAX_PARALLEL);

  url = (char *)malloc(sizeof(char)*1024);

  for(transfers = 0; transfers < MAX_PARALLEL; transfers++) {
    memset(url, 0, sizeof(char)*1024);
    sprintf(url, "https://hacker-news.firebaseio.com/v0/item/%i.json", item_id);
    item_id++;

    //printf("adding: %s\n", url);

    add_transfer(cm, transfers, url);
  }

  do {
    curl_multi_perform(cm, &still_alive);

    while((msg = curl_multi_info_read(cm, &msgs_left))) {
      if(msg->msg == CURLMSG_DONE) {
        char *url;
        CURL *e = msg->easy_handle;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &url);
        //fprintf(stderr, "R: %d - %s <%s>\n", msg->data.result, curl_easy_strerror(msg->data.result), url);
        curl_multi_remove_handle(cm, e);
        curl_easy_cleanup(e);
      }
      else {
        fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
      }
      //if(transfers < NUM_URLS) add_transfer(cm, transfers++);
      if (transfers < n_url) {
        memset(url, 0, sizeof(char)*1024);
        sprintf(url, "https://hacker-news.firebaseio.com/v0/item/%i.json", item_id);
        item_id++;
        transfers++;
        //printf("adding: %s\n", url);

        add_transfer(cm, transfers, url);
      }
    }
    if(still_alive) {
      curl_multi_wait(cm, NULL, 0, 1000, NULL);
    }

  } while(still_alive || (transfers < n_url));

  curl_multi_cleanup(cm);
  curl_global_cleanup();

  return EXIT_SUCCESS;
}
