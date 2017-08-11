/********************************************************************\

  Name:         ELOG.C
  Created by:   Stefan Ritt

  Contents:     MIDAS elog functions

  $Id$

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#ifndef HAVE_STRLCPY
#include "strlcpy.h"
#endif
#include <assert.h>

/** @defgroup elfunctioncode Midas Elog Functions (el_xxx)
 */

/**dox***************************************************************/
/** @addtogroup elfunctioncode
 *
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************\
*                                                                    *
*               Electronic logbook functions                         *
*                                                                    *
\********************************************************************/

/********************************************************************/
static void el_decode(const char *message, const char *key, char *result, int size)
{
   int i;
   const char *pc;

   if (result == NULL)
      return;

   *result = 0;

   pc = strstr(message, key);

   if (!pc)
     return;

   pc += strlen(key);

   for (i=0; i<size; i++) {
     if (pc[i] == 0)
       break;
     if (pc[i] == '\n')
       break;
     result[i] = pc[i];
   }

   assert(i<=size); // ensure that code above did not overrun the "result" array

   if (i==size)
     result[size-1] = 0;
   else
     result[i] = 0;
}

static void xwrite(const char* filename, int fd, const void* data, int size)
{
   int wr = write(fd, data, size);
   if (wr != size) {
      cm_msg(MERROR, "xwrite", "cannot write to \'%s\', write(%d) returned %d, errno %d (%s)", filename, size, wr, errno, strerror(errno));
   }
}

static int xread(const char* filename, int fd, void* data, int size)
{
   int rd = read(fd, data, size);
   if (rd == 0) {
      // end of file
   } else if (rd < 0) {
      cm_msg(MERROR, "xread", "cannot read from \'%s\', read(%d) returned %d, errno %d (%s)", filename, size, rd, errno, strerror(errno));
   } else if (rd != size) {
      cm_msg(MERROR, "xread", "truncated read from \'%s\', read(%d) returned %d", filename, size, rd);
   }
   return rd;
}

static void xtruncate(const char* filename, int fd)
{
   int tr = ftruncate(fd, TELL(fd));
   if (tr != 0) {
      cm_msg(MERROR, "xtruncate", "cannot truncate \'%s\', ftruncate() returned %d, errno %d (%s)", filename, tr, errno, strerror(errno));
   }
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Submit an ELog entry.
@param run  Run Number.
@param author Message author.
@param type Message type.
@param syst Message system.
@param subject Subject.
@param text Message text.
@param reply_to In reply to this message.
@param encoding Text encoding, either HTML or plain.
@param afilename1   File name of attachment.
@param buffer1      File contents.
@param buffer_size1 Size of buffer in bytes.
@param afilename2   File name of attachment.
@param buffer2      File contents.
@param buffer_size2 Size of buffer in bytes.
@param afilename3   File name of attachment.
@param buffer3      File contents.
@param buffer_size3 Size of buffer in bytes.
@param tag          If given, edit existing message.
@param tag_size     Maximum size of tag.
@return EL_SUCCESS
*/
INT el_submit(int run, const char *author, const char *type, const char *syst, const char *subject,
              const char *text, const char *reply_to, const char *encoding,
              const char *afilename1, char *buffer1, INT buffer_size1,
              const char *afilename2, char *buffer2, INT buffer_size2,
              const char *afilename3, char *buffer3, INT buffer_size3, char *tag, INT tag_size)
{
   if (rpc_is_remote())
      return rpc_call(RPC_EL_SUBMIT, run, author, type, syst, subject,
                      text, reply_to, encoding,
                      afilename1, buffer1, buffer_size1,
                      afilename2, buffer2, buffer_size2, afilename3, buffer3, buffer_size3, tag, tag_size);

#ifdef LOCAL_ROUTINES
   {
      INT size, fh, status, run_number, semaphore, buffer_size = 0, idx, offset = 0, tail_size = 0;
      struct tm *tms = NULL;
      char afilename[256], file_name[256], afile_name[3][256], dir[256], str[256],
          start_str[80], end_str[80], last[80], date[80], thread[80], attachment[256];
      HNDLE hDB;
      time_t now;
      char *p;
      char* message = NULL;
      size_t message_size = 0;
      char *buffer = NULL;
      BOOL bedit;

      cm_get_experiment_database(&hDB, NULL);

      bedit = (tag[0] != 0);

      /* request semaphore */
      cm_get_experiment_semaphore(NULL, &semaphore, NULL, NULL);
      status = ss_semaphore_wait_for(semaphore, 5 * 60 * 1000);
      if (status != SS_SUCCESS) {
         cm_msg(MERROR, "el_submit", "Cannot lock experiment semaphore, ss_semaphore_wait_for() status %d", status);
         abort();
      }

      /* get run number from ODB if not given */
      if (run > 0)
         run_number = run;
      else {
         /* get run number */
         size = sizeof(run_number);
         status = db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT, TRUE);
         assert(status == SUCCESS);
      }

      if (run_number < 0) {
         cm_msg(MERROR, "el_submit", "aborting on attempt to use invalid run number %d", run_number);
         abort();
      }

      for (idx = 0; idx < 3; idx++) {
         /* generate filename for attachment */
         afile_name[idx][0] = file_name[0] = 0;

         if (idx == 0) {
            strcpy(afilename, afilename1);
            buffer = buffer1;
            buffer_size = buffer_size1;
         } else if (idx == 1) {
            strcpy(afilename, afilename2);
            buffer = buffer2;
            buffer_size = buffer_size2;
         } else if (idx == 2) {
            strcpy(afilename, afilename3);
            buffer = buffer3;
            buffer_size = buffer_size3;
         }

         if (afilename[0]) {
            strcpy(file_name, afilename);
            p = file_name;
            while (strchr(p, ':'))
               p = strchr(p, ':') + 1;
            while (strchr(p, '\\'))
               p = strchr(p, '\\') + 1; /* NT */
            while (strchr(p, '/'))
               p = strchr(p, '/') + 1;  /* Unix */
            while (strchr(p, ']'))
               p = strchr(p, ']') + 1;  /* VMS */

            /* assemble ELog filename */
            if (p[0]) {
               dir[0] = 0;
               if (hDB > 0) {
                  size = sizeof(dir);
                  memset(dir, 0, size);
                  status = db_get_value(hDB, 0, "/Logger/Elog dir", dir, &size, TID_STRING, FALSE);
                  if (status != DB_SUCCESS)
                     db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING, TRUE);

                  if (dir[0] != 0 && dir[strlen(dir) - 1] != DIR_SEPARATOR)
                     strcat(dir, DIR_SEPARATOR_STR);
               }
#if !defined(OS_VXWORKS)
#if !defined(OS_VMS)
               tzset();
#endif
#endif

               time(&now);
               tms = localtime(&now);

               strcpy(str, p);
               sprintf(afile_name[idx], "%02d%02d%02d_%02d%02d%02d_%s",
                       tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday,
                       tms->tm_hour, tms->tm_min, tms->tm_sec, str);
               sprintf(file_name, "%s%02d%02d%02d_%02d%02d%02d_%s", dir,
                       tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday,
                       tms->tm_hour, tms->tm_min, tms->tm_sec, str);

               /* save attachment */
               fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
               if (fh < 0) {
                  cm_msg(MERROR, "el_submit", "Cannot write attachment file \"%s\", open() returned %d, errno %d (%s)", file_name, fh, errno, strerror(errno));
               } else {
                  xwrite(file_name, fh, buffer, buffer_size);
                  close(fh);
               }
            }
         }
      }

      /* generate new file name YYMMDD.log in data directory */
      cm_get_experiment_database(&hDB, NULL);

      size = sizeof(dir);
      memset(dir, 0, size);
      status = db_get_value(hDB, 0, "/Logger/Elog dir", dir, &size, TID_STRING, FALSE);
      if (status != DB_SUCCESS)
         db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING, TRUE);

      if (dir[0] != 0 && dir[strlen(dir) - 1] != DIR_SEPARATOR)
         strcat(dir, DIR_SEPARATOR_STR);

#if !defined(OS_VXWORKS)
#if !defined(OS_VMS)
      tzset();
#endif
#endif

      if (bedit) {
         /* edit existing message */
         strcpy(str, tag);
         if (strchr(str, '.')) {
            offset = atoi(strchr(str, '.') + 1);
            *strchr(str, '.') = 0;
         }
         sprintf(file_name, "%s%s.log", dir, str);
         fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
         if (fh < 0) {
            ss_semaphore_release(semaphore);
            return EL_FILE_ERROR;
         }
         lseek(fh, offset, SEEK_SET);
         xread(file_name, fh, str, 16);

         if (strncmp(str, "$Start$", 7) != 0) {
            cm_msg(MERROR, "el_submit", "cannot read from \'%s\', corrupted file: no $Start$ in \"%s\"", file_name, str);
            close(fh);
            return EL_FILE_ERROR;
         }

         size = atoi(str + 9);

         if (size < 1) {
            cm_msg(MERROR, "el_submit", "cannot read from \'%s\', corrupted file: bad size %d in \"%s\"", file_name, size, str);
            close(fh);
            return EL_FILE_ERROR;
         }

         message = (char*)malloc(size);

         if (!message) {
            cm_msg(MERROR, "el_submit", "cannot read from \'%s\', corrupted file: bad size %d in \"%s\", cannot malloc(%d): errno %d (%s)", file_name, size, str, size, errno, strerror(errno));
            close(fh);
            return EL_FILE_ERROR;
         }

         status = read(fh, message, size);
         if (status != size && status + 16 != size) {
            free(message);
            return EL_FILE_ERROR;
         }

         el_decode(message, "Date: ", date, sizeof(date));
         el_decode(message, "Thread: ", thread, sizeof(thread));
         el_decode(message, "Attachment: ", attachment, sizeof(attachment));

         free(message);
         message = NULL;

         /* buffer tail of logfile */
         lseek(fh, 0, SEEK_END);
         tail_size = TELL(fh) - (offset + size);

         if (tail_size > 0) {
            int n;

            buffer = (char *) M_MALLOC(tail_size);
            if (buffer == NULL) {
               close(fh);
               ss_semaphore_release(semaphore);
               return EL_FILE_ERROR;
            }

            lseek(fh, offset + size, SEEK_SET);
            n = xread(file_name, fh, buffer, tail_size);
            if (n != tail_size)
               return EL_FILE_ERROR;
         }
         lseek(fh, offset, SEEK_SET);
      } else {
         /* create new message */
         time(&now);
         tms = localtime(&now);

         sprintf(file_name, "%s%02d%02d%02d.log", dir, tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday);

         fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
         if (fh < 0) {
            ss_semaphore_release(semaphore);
            return EL_FILE_ERROR;
         }

         strcpy(date, ctime(&now));
         date[24] = 0;

         if (reply_to[0])
            sprintf(thread, "%16s %16s", reply_to, "0");
         else
            sprintf(thread, "%16s %16s", "0", "0");

         lseek(fh, 0, SEEK_END);
      }

      message_size = 1000;
      message_size += strlen(date);
      message_size += strlen(author);
      message_size += strlen(type);
      message_size += strlen(syst);
      message_size += strlen(subject);
      message_size += strlen(attachment);
      message_size += strlen(afile_name[0]);
      message_size += strlen(afile_name[1]);
      message_size += strlen(afile_name[2]);
      message_size += strlen(encoding);
      message_size += strlen(text);

      //printf("message_size %d, text %d\n", (int)message_size, (int)strlen(text));

      message = (char*)malloc(message_size);

      if (!message) {
         cm_msg(MERROR, "el_submit", "cannot malloc() %d bytes: errno %d (%s)", size, errno, strerror(errno));
         close(fh);
         return EL_FILE_ERROR;
      }

      /* compose message */

      sprintf(message, "Date: %s\n", date);
      sprintf(message + strlen(message), "Thread: %s\n", thread);
      sprintf(message + strlen(message), "Run: %d\n", run_number);
      sprintf(message + strlen(message), "Author: %s\n", author);
      sprintf(message + strlen(message), "Type: %s\n", type);
      sprintf(message + strlen(message), "System: %s\n", syst);
      sprintf(message + strlen(message), "Subject: %s\n", subject);

      /* keep original attachment if edit and no new attachment */
      if (bedit && afile_name[0][0] == 0 && afile_name[1][0] == 0 && afile_name[2][0] == 0)
         sprintf(message + strlen(message), "Attachment: %s", attachment);
      else {
         sprintf(message + strlen(message), "Attachment: %s", afile_name[0]);
         if (afile_name[1][0])
            sprintf(message + strlen(message), ",%s", afile_name[1]);
         if (afile_name[2][0])
            sprintf(message + strlen(message), ",%s", afile_name[2]);
      }
      sprintf(message + strlen(message), "\n");

      sprintf(message + strlen(message), "Encoding: %s\n", encoding);
      sprintf(message + strlen(message), "========================================\n");
      strcat(message, text);

      assert(strlen(message) < message_size);        /* bomb out on array overrun. */

      size = 0;
      sprintf(start_str, "$Start$: %6d\n", size);
      sprintf(end_str, "$End$:   %6d\n\f", size);

      size = strlen(message) + strlen(start_str) + strlen(end_str);

      if (tag != NULL && !bedit)
         sprintf(tag, "%02d%02d%02d.%d", tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday, (int) TELL(fh));

      /* size has to fit in 6 digits */
      assert(size < 999999);

      sprintf(start_str, "$Start$: %6d\n", size);
      sprintf(end_str, "$End$:   %6d\n\f", size);

      xwrite(file_name, fh, start_str, strlen(start_str));
      xwrite(file_name, fh, message, strlen(message));
      xwrite(file_name, fh, end_str, strlen(end_str));

      free(message);
      message = NULL;
      message_size = 0;

      if (bedit) {
         if (tail_size > 0) {
            xwrite(file_name, fh, buffer, tail_size);
            M_FREE(buffer);
         }

         /* truncate file here */
#ifdef OS_WINNT
         chsize(fh, TELL(fh));
#else
         xtruncate(file_name, fh);
#endif
      }

      close(fh);

      /* if reply, mark original message */
      if (reply_to[0] && !bedit) {
         strcpy(last, reply_to);
         do {
            char filename[256];
            status = el_search_message(last, &fh, FALSE, filename, sizeof(filename));
            if (status == EL_SUCCESS) {
               /* position to next thread location */
               lseek(fh, 72, SEEK_CUR);
               memset(str, 0, sizeof(str));
               xread(filename, fh, str, 16);
               lseek(fh, -16, SEEK_CUR);

               /* if no reply yet, set it */
               if (atoi(str) == 0) {
                  sprintf(str, "%16s", tag);
                  xwrite(filename, fh, str, 16);
                  close(fh);
                  break;
               } else {
                  /* if reply set, find last one in chain */
                  strcpy(last, strtok(str, " "));
                  close(fh);
               }
            } else
               /* stop on error */
               break;

         } while (TRUE);
      }

      /* release elog semaphore */
      ss_semaphore_release(semaphore);
   }
#endif                          /* LOCAL_ROUTINES */

   return EL_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT el_search_message(char *tag, int *fh, BOOL walk, char *xfilename, int xfilename_size)
{
   int i, size, offset, direction, status;
   struct tm *tms, ltms;
   time_t lt, ltime=0, lact;
   char str[256], file_name[256], dir[256];
   HNDLE hDB;

#if !defined(OS_VXWORKS)
#if !defined(OS_VMS)
   tzset();
#endif
#endif

   if (xfilename && xfilename_size > 0)
      *xfilename = 0;

   /* open file */
   cm_get_experiment_database(&hDB, NULL);

   size = sizeof(dir);
   memset(dir, 0, size);
   status = db_get_value(hDB, 0, "/Logger/Elog dir", dir, &size, TID_STRING, FALSE);
   if (status != DB_SUCCESS)
      db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING, TRUE);

   if (dir[0] != 0 && dir[strlen(dir) - 1] != DIR_SEPARATOR)
      strcat(dir, DIR_SEPARATOR_STR);

   /* check tag for direction */
   direction = 0;
   if (strpbrk(tag, "+-")) {
      direction = atoi(strpbrk(tag, "+-"));
      *strpbrk(tag, "+-") = 0;
   }

   /* if tag is given, open file directly */
   if (tag[0]) {
      /* extract time structure from tag */
      tms = &ltms;
      memset(tms, 0, sizeof(struct tm));
      tms->tm_year = (tag[0] - '0') * 10 + (tag[1] - '0');
      tms->tm_mon = (tag[2] - '0') * 10 + (tag[3] - '0') - 1;
      tms->tm_mday = (tag[4] - '0') * 10 + (tag[5] - '0');
      tms->tm_hour = 12;

      if (tms->tm_year < 90)
         tms->tm_year += 100;
      ltime = lt = mktime(tms);

      strcpy(str, tag);
      if (strchr(str, '.')) {
         offset = atoi(strchr(str, '.') + 1);
         *strchr(str, '.') = 0;
      } else
         return EL_FILE_ERROR;

      do {
         tms = localtime(&ltime);

         sprintf(file_name, "%s%02d%02d%02d.log", dir, tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday);

         if (xfilename)
            strlcpy(xfilename, file_name, xfilename_size);

         *fh = open(file_name, O_RDWR | O_BINARY, 0644);

         if (*fh < 0) {
            if (!walk)
               return EL_FILE_ERROR;

            if (direction == -1)
               ltime -= 3600 * 24;      /* one day back */
            else
               ltime += 3600 * 24;      /* go forward one day */

            /* set new tag */
            tms = localtime(&ltime);
            sprintf(tag, "%02d%02d%02d.0", tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday);
         }

         /* in forward direction, stop today */
         if (direction != -1 && ltime > time(NULL) + 3600 * 24)
            break;

         /* in backward direction, go back 10 years */
         if (direction == -1 && abs((INT) lt - (INT) ltime) > 3600 * 24 * 365 * 10)
            break;

      } while (*fh < 0);

      if (*fh < 0)
         return EL_FILE_ERROR;

      lseek(*fh, offset, SEEK_SET);

      /* check if start of message */
      i = xread(file_name, *fh, str, 15);
      if (i <= 0) {
         close(*fh);
         return EL_FILE_ERROR;
      }

      if (strncmp(str, "$Start$: ", 9) != 0) {
         close(*fh);
         return EL_FILE_ERROR;
      }

      lseek(*fh, offset, SEEK_SET);
   }

   /* open most recent file if no tag given */
   if (tag[0] == 0) {
      time((time_t *) &lt);
      ltime = lt;
      do {
         tms = localtime(&ltime);

         sprintf(file_name, "%s%02d%02d%02d.log", dir, tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday);

         if (xfilename)
            strlcpy(xfilename, file_name, xfilename_size);

         *fh = open(file_name, O_RDWR | O_BINARY, 0644);

         if (*fh < 0)
            ltime -= 3600 * 24; /* one day back */

      } while (*fh < 0 && (INT) lt - (INT) ltime < 3600 * 24 * 365);

      if (*fh < 0)
         return EL_FILE_ERROR;

      /* remember tag */
      sprintf(tag, "%02d%02d%02d", tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday);

      lseek(*fh, 0, SEEK_END);

      sprintf(tag + strlen(tag), ".%d", (int) TELL(*fh));
   }


   if (direction == -1) {
      /* seek previous message */

      if (TELL(*fh) == 0) {
         /* go back one day */
         close(*fh);

         lt = ltime;
         do {
            lt -= 3600 * 24;
            tms = localtime(&lt);
            sprintf(str, "%02d%02d%02d.0", tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday);

            status = el_search_message(str, fh, FALSE, file_name, sizeof(file_name));

            if (xfilename)
               strlcpy(xfilename, file_name, xfilename_size);

         } while (status != EL_SUCCESS && (INT) ltime - (INT) lt < 3600 * 24 * 365);

         if (status != EL_SUCCESS)
            return EL_FIRST_MSG;

         /* adjust tag */
         strcpy(tag, str);

         /* go to end of current file */
         lseek(*fh, 0, SEEK_END);
      }

      /* read previous message size */
      lseek(*fh, -17, SEEK_CUR);
      i = xread(file_name, *fh, str, 17);
      if (i <= 0) {
         close(*fh);
         return EL_FILE_ERROR;
      }

      if (strncmp(str, "$End$: ", 7) != 0) {
         close(*fh);
         return EL_FILE_ERROR;
      }

      /* make sure the input string to atoi() is zero-terminated:
       * $End$:      355garbage
       * 01234567890123456789 */
      str[15] = 0;

      size = atoi(str + 7);
      if (size <= 15) {
         close(*fh);
         return EL_FILE_ERROR;
      }

      lseek(*fh, -size, SEEK_CUR);

      /* adjust tag */
      sprintf(strchr(tag, '.') + 1, "%d", (int) TELL(*fh));
   }

   if (direction == 1) {
      /* seek next message */

      /* read current message size */
      TELL(*fh);

      i = xread(file_name, *fh, str, 15);
      if (i <= 0) {
         close(*fh);
         return EL_FILE_ERROR;
      }
      lseek(*fh, -15, SEEK_CUR);

      if (strncmp(str, "$Start$: ", 9) != 0) {
         close(*fh);
         return EL_FILE_ERROR;
      }

      /* make sure the input string to atoi() is zero-terminated
       * $Start$:    606garbage
       * 01234567890123456789 */
      str[15] = 0;

      size = atoi(str + 9);

      if (size <= 15) {
         close(*fh);
         return EL_FILE_ERROR;
      }

      lseek(*fh, size, SEEK_CUR);

      /* if EOF, goto next day */
      i = xread(file_name, *fh, str, 15);
      if (i < 15) {
         close(*fh);
         time((time_t *) &lact);

         lt = ltime;
         do {
            lt += 3600 * 24;
            tms = localtime(&lt);
            sprintf(str, "%02d%02d%02d.0", tms->tm_year % 100, tms->tm_mon + 1, tms->tm_mday);

            status = el_search_message(str, fh, FALSE, file_name, sizeof(file_name));

            if (xfilename)
               strlcpy(xfilename, file_name, xfilename_size);

         } while (status != EL_SUCCESS && (INT) lt - (INT) lact < 3600 * 24);

         if (status != EL_SUCCESS)
            return EL_LAST_MSG;

         /* adjust tag */
         strcpy(tag, str);

         /* go to beginning of current file */
         lseek(*fh, 0, SEEK_SET);
      } else
         lseek(*fh, -15, SEEK_CUR);

      /* adjust tag */
      sprintf(strchr(tag, '.') + 1, "%d", (int) TELL(*fh));
   }

   return EL_SUCCESS;
}


/********************************************************************/
INT el_retrieve(char *tag, char *date, int *run, char *author, char *type,
                char *syst, char *subject, char *text, int *textsize,
                char *orig_tag, char *reply_tag,
                char *attachment1, char *attachment2, char *attachment3, char *encoding)
/********************************************************************\

  Routine: el_retrieve

  Purpose: Retrieve an ELog entry by its message tab

  Input:
    char   *tag             tag in the form YYMMDD.offset
    int    *size            Size of text buffer

  Output:
    char   *tag             tag of retrieved message
    char   *date            Date/time of message recording
    int    *run             Run number
    char   *author          Message author
    char   *type            Message type
    char   *syst            Message system
    char   *subject         Subject
    char   *text            Message text
    char   *orig_tag        Original message if this one is a reply
    char   *reply_tag       Reply for current message
    char   *attachment1/2/3 File attachment
    char   *encoding        Encoding of message
    int    *size            Actual message text size

  Function value:
    EL_SUCCESS              Successful completion
    EL_LAST_MSG             Last message in log

\********************************************************************/
{
   int size, fh = 0, search_status, rd;
   char str[256], *p;
   char thread[256];
   char attachment_all[3*256+100]; /* size of attachement1/2/3 from show_elog_submit_query() */
   char *message = NULL;
   size_t message_size = 0;
   char filename[256];

   if (tag[0]) {
      search_status = el_search_message(tag, &fh, TRUE, filename, sizeof(filename));
      if (search_status != EL_SUCCESS)
         return search_status;
   } else {
      /* open most recent message */
      strcpy(tag, "-1");
      search_status = el_search_message(tag, &fh, TRUE, filename, sizeof(filename));
      if (search_status != EL_SUCCESS)
         return search_status;
   }

   //printf("el_retrieve: reading [%s]\n", filename);

   /* extract message size */
   TELL(fh);
   rd = xread(filename, fh, str, 15);
   if (rd != 15)
      return EL_FILE_ERROR;

   /* make sure the input string is zero-terminated before we call atoi() */
   str[15] = 0;

   /* get size */
   size = atoi(str + 9);

   if ((strncmp(str, "$Start$:", 8) != 0) || (size <= 15)) {
      cm_msg(MERROR, "el_retrieve", "cannot read from \'%s\', corrupted file: no $Start$ or bad size in \"%s\"", filename, str);
      close(fh);
      return EL_FILE_ERROR;
   }

   message_size = size + 1;
   message = (char*)malloc(message_size);

   if (!message) {
      cm_msg(MERROR, "el_retrieve", "cannot read from \'%s\', cannot malloc() %d bytes, errno %d (%s)", filename, (int)message_size, errno, strerror(errno));
      free(message);
      close(fh);
      return EL_FILE_ERROR;
   }

   memset(message, 0, message_size);

   rd = read(fh, message, size);
   if (rd <= 0 || !((rd + 15 == size) || (rd == size))) {
      cm_msg(MERROR, "el_retrieve", "cannot read from \'%s\', read(%d) returned %d, errno %d (%s)", filename, size, rd, errno, strerror(errno));
      free(message);
      close(fh);
      return EL_FILE_ERROR;
   }

   close(fh);

   /* decode message */
   if (strstr(message, "Run: ") && run)
      *run = atoi(strstr(message, "Run: ") + 5);

   el_decode(message, "Date: ", date, 80);      /* size from show_elog_submit_query() */
   el_decode(message, "Thread: ", thread, sizeof(thread));
   el_decode(message, "Author: ", author, 80);  /* size from show_elog_submit_query() */
   el_decode(message, "Type: ", type, 80);      /* size from show_elog_submit_query() */
   el_decode(message, "System: ", syst, 80);  /* size from show_elog_submit_query() */
   el_decode(message, "Subject: ", subject, 256);       /* size from show_elog_submit_query() */
   el_decode(message, "Attachment: ", attachment_all, sizeof(attachment_all));
   el_decode(message, "Encoding: ", encoding, 80);      /* size from show_elog_submit_query() */

   /* break apart attachements */
   if (attachment1 && attachment2 && attachment3) {
      attachment1[0] = attachment2[0] = attachment3[0] = 0;
      p = strtok(attachment_all, ",");
      if (p != NULL) {
         strlcpy(attachment1, p, 256);          /* size from show_elog_submit_query() */
         p = strtok(NULL, ",");
         if (p != NULL) {
            strlcpy(attachment2, p, 256);       /* size from show_elog_submit_query() */
            p = strtok(NULL, ",");
            if (p != NULL)
               strlcpy(attachment3, p, 256);    /* size from show_elog_submit_query() */
         }
      }
   }

   /* conver thread in reply-to and reply-from */
   if (orig_tag != NULL && reply_tag != NULL) {
      p = strtok(thread, " \r");
      if (p != NULL)
         strcpy(orig_tag, p);
      else
         strcpy(orig_tag, "");
      p = strtok(NULL, " \r");
      if (p != NULL)
         strcpy(reply_tag, p);
      else
         strcpy(reply_tag, "");
      if (atoi(orig_tag) == 0)
         orig_tag[0] = 0;
      if (atoi(reply_tag) == 0)
         reply_tag[0] = 0;
   }

   p = strstr(message, "========================================\n");

   if (text != NULL) {
      if (p != NULL) {
         p += 41;
         if ((int) strlen(p) >= *textsize) {
            strncpy(text, p, *textsize - 1);
            text[*textsize - 1] = 0;
            free(message);
            return EL_TRUNCATED;
         } else {
            strcpy(text, p);

            /* strip end tag */
            if (strstr(text, "$End$"))
               *strstr(text, "$End$") = 0;

            *textsize = strlen(text);
         }
      } else {
         text[0] = 0;
         *textsize = 0;
      }
   }

   free(message);
   message = NULL;

   if (search_status == EL_LAST_MSG)
      return EL_LAST_MSG;

   return EL_SUCCESS;
}


/********************************************************************/
INT el_search_run(int run, char *return_tag)
/********************************************************************\

  Routine: el_search_run

  Purpose: Find first message belonging to a specific run

  Input:
    int    run              Run number

  Output:
    char   *tag             tag of retrieved message

  Function value:
    EL_SUCCESS              Successful completion
    EL_LAST_MSG             Last message in log

\********************************************************************/
{
   int actual_run=0, fh, status;
   char tag[256];

   tag[0] = return_tag[0] = 0;

   do {
      /* open first message in file */
      strcat(tag, "-1");
      status = el_search_message(tag, &fh, TRUE, NULL, 0);
      if (status == EL_FIRST_MSG)
         break;
      if (status != EL_SUCCESS)
         return status;
      close(fh);

      if (strchr(tag, '.') != NULL)
         strcpy(strchr(tag, '.'), ".0");

      el_retrieve(tag, NULL, &actual_run, NULL, NULL,
                  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
   } while (actual_run >= run);

   while (actual_run < run) {
      strcat(tag, "+1");
      status = el_search_message(tag, &fh, TRUE, NULL, 0);
      if (status == EL_LAST_MSG)
         break;
      if (status != EL_SUCCESS)
         return status;
      close(fh);

      el_retrieve(tag, NULL, &actual_run, NULL, NULL,
                  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
   }

   strcpy(return_tag, tag);

   if (status == EL_LAST_MSG || status == EL_FIRST_MSG)
      return status;

   return EL_SUCCESS;
}


/********************************************************************/
INT el_delete_message(const char *tag)
/********************************************************************\

  Routine: el_submit

  Purpose: Submit an ELog entry

  Input:
    char   *tag             Message tage

  Output:
    <none>

  Function value:
    EL_SUCCESS              Successful completion

\********************************************************************/
{
#ifdef LOCAL_ROUTINES
   INT size, fh, semaphore, offset = 0, tail_size, status;
   char dir[256], str[256], file_name[256];
   HNDLE hDB;
   char *buffer = NULL;

   cm_get_experiment_database(&hDB, NULL);

   /* request semaphore */
   cm_get_experiment_semaphore(NULL, &semaphore, NULL, NULL);
   status = ss_semaphore_wait_for(semaphore, 5 * 60 * 1000);
   if (status != SS_SUCCESS) {
      cm_msg(MERROR, "el_delete_message",
             "Cannot lock experiment semaphore, ss_semaphore_wait_for() status %d", status);
      abort();
   }

   /* generate file name YYMMDD.log in data directory */
   cm_get_experiment_database(&hDB, NULL);

   size = sizeof(dir);
   memset(dir, 0, size);
   status = db_get_value(hDB, 0, "/Logger/Elog dir", dir, &size, TID_STRING, FALSE);
   if (status != DB_SUCCESS)
      db_get_value(hDB, 0, "/Logger/Data dir", dir, &size, TID_STRING, TRUE);

   if (dir[0] != 0 && dir[strlen(dir) - 1] != DIR_SEPARATOR)
      strcat(dir, DIR_SEPARATOR_STR);

   strcpy(str, tag);
   if (strchr(str, '.')) {
      offset = atoi(strchr(str, '.') + 1);
      *strchr(str, '.') = 0;
   }
   sprintf(file_name, "%s%s.log", dir, str);
   fh = open(file_name, O_CREAT | O_RDWR | O_BINARY, 0644);
   if (fh < 0) {
      ss_semaphore_release(semaphore);
      return EL_FILE_ERROR;
   }
   lseek(fh, offset, SEEK_SET);
   xread(file_name, fh, str, 16);
   size = atoi(str + 9);

   /* buffer tail of logfile */
   lseek(fh, 0, SEEK_END);
   tail_size = TELL(fh) - (offset + size);

   if (tail_size > 0) {
      buffer = (char *) M_MALLOC(tail_size);
      if (buffer == NULL) {
         close(fh);
         ss_semaphore_release(semaphore);
         return EL_FILE_ERROR;
      }

      lseek(fh, offset + size, SEEK_SET);
      xread(file_name, fh, buffer, tail_size);
   }
   lseek(fh, offset, SEEK_SET);

   if (tail_size > 0) {
      xwrite(file_name, fh, buffer, tail_size);
      M_FREE(buffer);
   }

   /* truncate file here */
#ifdef OS_WINNT
   chsize(fh, TELL(fh));
#else
   xtruncate(file_name, fh);
#endif

   /* if file length gets zero, delete file */
   tail_size = lseek(fh, 0, SEEK_END);
   close(fh);

   if (tail_size == 0)
      remove(file_name);

   /* release elog semaphore */
   ss_semaphore_release(semaphore);
#endif                          /* LOCAL_ROUTINES */

   return EL_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
/** @} *//* end of elfunctioncode */

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
