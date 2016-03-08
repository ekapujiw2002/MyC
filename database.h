/*
   *
   * database.h - simple routines for accessing MySQL databases from C
   * Contains MyC routines
   *
   * Written 03/99 by Ed Carp, erc@pobox.com
   * Copyright 1999 by Ed Carp
   * Use of this code is governed by your MySQL License Agreement.
   *
 */
#include <stdio.h>
#include <stdlib.h>
#include "mysql.h"
#define MYC_VERSION "0.0.1 ALPHA"
#undef NULL
#undef EOF
#define NULL (0)
#define EOF (-1)
#define TRUE EOF
#define FALSE NULL

/*
   #define DEBUG
 */

#define SetFieldLimit 8192	/* currently unused */
#define M_UPDATE 0
#define M_DELETE 1

#define FieldValue(n) m_field_values[n]
#define FieldName(n) m_fields[n].name

MYSQL mysql;
int mysql_valid;
MYSQL_ROW m_row;
int m_row_valid;
MYSQL_RES *m_result;
int m_result_valid;
MYSQL_FIELD *m_fields;
int m_fields_valid;
int m_add_flag, m_update_flag;
char m_current_query[4096];
int m_EOF;

unsigned int m_row_tell, AbsolutePosition (), RecordCount (), FieldCount ();
char *parse_query (char *lookfor);
char **m_field_values;
char *strstr ();
long *m_field_lengths;
int m_binary_data;

int
OpenDatabase (db, host, name, pass)
     char *db, *host, *name, *pass;
{
  if (mysql_valid == TRUE)
    CloseDatabase ();
  mysql_valid = m_row_valid = m_result_valid = m_fields_valid = FALSE;
  if (!mysql_connect (&mysql, host, name, pass))
    {
      fprintf (stderr, "ERROR connecting to database: %s\n",
	       mysql_error (&mysql));
      return (EOF);
    }
  if (mysql_select_db (&mysql, db))
    {
      fprintf (stderr, "ERROR selecting database: %s\n", mysql_error (&mysql));
      return (EOF);
    }
  m_EOF = TRUE;
  return (NULL);
}

int
OpenRecordset (query)
     char *query;
{
  int i, n;
  long l;
  char *p;

  if (m_result_valid == TRUE)
    {
      CloseRecordset ();
      m_result_valid = FALSE;
    }
  m_row_tell = 0;
  strncpy (m_current_query, query, 4096);
/*
   l = strlen (query);
   p = (char *) malloc (l * 2 + 1);
   mysql_escape_string (p, query, l);
 */
  if (mysql_query (&mysql, query))
    {
/*
   fprintf (stderr, "ERROR running query: %s\n", mysql_error (&mysql));
   free (p);
 */
      return (EOF);
    }
/*
   free (p);
 */
  if (!(m_result = mysql_store_result (&mysql)))
    {
/*
   fprintf (stderr, "ERROR getting query results: %s\n",
   mysql_error (&mysql));
 */
      return (EOF);
    }
  m_row = mysql_fetch_row (m_result);
  if (!(m_fields = mysql_fetch_fields (m_result)))
    {
      fprintf (stderr, "ERROR getting fields: %s\n",
	       mysql_error (&mysql));
      return (EOF);
    }
  m_field_values = (char **) malloc (mysql_num_fields (m_result) * sizeof (char *));
  m_field_lengths = (long *) malloc (mysql_num_fields (m_result) * sizeof (long));
  n = mysql_num_fields (m_result);
  for (i = 0; i < n; i++)
    m_field_values[i] = NULL;
  m_result_valid = TRUE;
  m_binary_data = FALSE;
  m_EOF = FALSE;
  if (RecordCount () == 0)
    m_EOF = TRUE;
  return (NULL);
}

int
MoveNext ()
{
  int rc;

  rc = RecordCount ();
  if (m_result_valid == FALSE)
    return (EOF);
  m_row_tell++;
  if (m_row_tell >= rc)
    {
      m_row_tell = rc - 1;
      m_EOF = TRUE;
      return (NULL);
    }
  if (!(m_row = mysql_fetch_row (m_result)))
    return (EOF);
  return (NULL);
}

int
MovePrev ()
{
  if (m_result_valid == FALSE)
    return (EOF);
  m_row_tell--;
  if (m_row_tell < 0)
    {
      m_row_tell = 0;
      m_EOF = TRUE;
      return (NULL);
    }
  mysql_data_seek (m_result, m_row_tell);
  if (!(m_row = mysql_fetch_row (m_result)))
    {
      m_EOF = TRUE;
      return (EOF);
    }
  return (NULL);
}

int
MoveLast ()
{
  if (m_result_valid == FALSE)
    return (EOF);
  m_row_tell = RecordCount () - 1;
  mysql_data_seek (m_result, m_row_tell);
  if (!(m_row = mysql_fetch_row (m_result)))
    return (EOF);
  m_EOF = FALSE;
  return (NULL);
}

int
MoveFirst ()
{
  if (m_result_valid == FALSE)
    return (EOF);
  mysql_data_seek (m_result, 0);
  m_row_tell = 0;
  if (!(m_row = mysql_fetch_row (m_result)))
    return (EOF);
  m_EOF = FALSE;
  return (NULL);
}

char *
GetField (fieldname)
     char *fieldname;
{
  int i, l;

  if (m_result_valid == FALSE || m_EOF == EOF)
    return ((char *) EOF);
  l = mysql_num_fields (m_result);
  for (i = 0; i < l; i++)
    if (strcmp (m_fields[i].name, fieldname) == 0)
      return (m_row[i]);
  return ((char *) EOF);
}

char *
GetFieldN (n)
     int n;
{
  int i, l;

  if (m_result_valid == FALSE || m_EOF == EOF)
    return ((char *) EOF);
  l = mysql_num_fields (m_result);
  if (n > -1 && n < l)
    return (m_row[n]);
  else
    return ((char *) EOF);
}

int
SetField (fieldname, value)
     char *fieldname, *value;
{
  int i, l;

  if (m_result_valid == FALSE)
    return (EOF);
  l = mysql_num_fields (m_result);
#ifdef DEBUG
  fprintf (stderr, "SetField: before: m_field_values[1]='%s'\n", m_field_values[1]);
#endif
  for (i = 0; i < l; i++)
    if (strcmp (m_fields[i].name, fieldname) == 0)
      {
	if (value != (char *) NULL)
	  {
	    if (*value != NULL)
	      {
		if (strlen (value) > 0)
		  {
#ifdef DEBUG
		    fprintf (stderr, "SetField: setting field %d to '%s'\n", i, value);
#endif
		    m_field_values[i] = value;
		    m_field_lengths[i] = strlen (value);
		  }
	      }
	  }
#ifdef DEBUG
	fprintf (stderr, "SetField: after: m_field_values[1]='%s'\n", m_field_values[1]);
#endif
	return (NULL);
      }
  return (EOF);
}

int
SetFieldN (fieldnum, value)
     int fieldnum;
     char *value;
{
  if (m_result_valid == FALSE)
    return (EOF);
  m_field_values[fieldnum] = value;
  m_field_lengths[fieldnum] = strlen (value);
}

/*
   * SetFieldB - convert to binary string IN PLACE.
   * 'value' must be big enough to hold the result.
 */
int
SetFieldB (fieldname, value)
     char *fieldname, *value;
{
  char *ptr;
  int l, s;

#ifdef DEBUG
  fprintf (stderr, "SetFieldB start: FieldValue(1)='%s'\n", FieldValue (1));
#endif
  s = strlen (value);
  ptr = (char *) malloc (s * 2 + 1);
  l = mysql_escape_string (ptr, value, s);
  *(ptr + l + 1) = NULL;
  strcpy (value, ptr);
  free (ptr);
#ifdef DEBUG
  fprintf (stderr, "SetFieldB before SetFieldB call: FieldValue(1)='%s'\n", FieldValue (1));
  fprintf (stderr, "calling SetField('%s', '%s')\n", fieldname, value);
#endif
  SetField (fieldname, value);
#ifdef DEBUG
  fprintf (stderr, "SetFieldB end: FieldValue(1)='%s'\n", FieldValue (1));
#endif
}

Update (table)
     char *table;
{
  int ret, l;

/*
  l = m_row_tell;
*/
  ret = _m_update_or_delete (table, M_UPDATE);
/*
  mysql_data_seek (m_result, l);
*/
  return (ret);
}

Delete (table)
     char *table;
{
  int i;
  for (i = 0; i < FieldCount (); i++)
    SetFieldN (i, GetFieldN (i));
  return (_m_update_or_delete (table, M_DELETE));
}

_m_update_or_delete (table, flag)
     char *table;
     int flag;
{
  int i, l, added;
  long n, nl, tot;
  char *query, *p;

  if (m_result_valid == FALSE)
    return (EOF);
  l = mysql_num_fields (m_result);
#ifdef DEBUG
  fprintf (stderr, "Update: m_field_values[1]='%s'\n", m_field_values[1]);
  fprintf (stderr, "Update: l=%d, update flag=%d\n", l, m_update_flag);
#endif
  if (m_update_flag == TRUE && (flag == M_UPDATE || flag == M_DELETE))
    {
#ifdef DEBUG
      fprintf (stderr, "Update: update flag=TRUE\n");
      fflush (stderr);
#endif
      tot = 0L;
      for (i = 0; i < l; i++)
	{
	  if (m_field_values[i] != (char *) NULL)
	    {
#ifdef DEBUG
	      fprintf (stderr, "Update: preparing to work on m_field_values[%d]\n", i);
	      fflush (stderr);
	      fprintf (stderr, "l=%d, ", l);
	      fflush (stderr);
	      fprintf (stderr, "tot=%ld, ", tot);
	      fflush (stderr);
	      fprintf (stderr, "i=%d, ", i);
	      fflush (stderr);
	      fprintf (stderr, "name='%s', ", m_fields[i].name);
	      fflush (stderr);
	      fprintf (stderr, "old value='%s', ", m_row[i]);
	      fflush (stderr);
	      fprintf (stderr, "new value='%s'\n", m_field_values[i]);
	      fflush (stderr);
#endif
	      tot += m_field_lengths[i] * 2 + 1;
	      if (m_row[i] != (char *) NULL)
		tot += strlen (m_row[i]) * 2 + 1;
	      tot += strlen (m_field_values[i]) * 2 + 1;
	      tot += strlen (m_fields[i].name) * 2;
	      tot += 5L;
	    }
	  else
	    tot += strlen (m_fields[i].name) * 2;
	  if (m_row[i] != (char *) NULL)
	    {
#ifdef DEBUG
	      fprintf (stderr, "Update: preparing to work on m_row[%d]\n", i);
	      fflush (stdout);
	      fprintf (stderr, "Update: adding %d to tot from m_row[%d]\n",
		       strlen (m_row[i]) * 2 + 1, i);
	      fflush (stdout);
#endif
	      tot += strlen (m_row[i]) * 2 + 1;
	    }
	}
      tot += 100L;
#ifdef DEBUG
      fprintf (stderr, "Update: allocating %ld bytes for query\n", tot);
      fflush (stderr);
#endif
      query = (char *) malloc (tot);
      if (m_update_flag == TRUE || flag == M_UPDATE)
	{
	  sprintf (query, "UPDATE %s SET ", table);
#ifdef DEBUG
	  fprintf (stderr, "Update: partial query='%s'\n", query);
	  fflush (stderr);
#endif
	  added = 0;
	  for (i = 0; i < l; i++)
	    {
	      if (m_field_values[i] != NULL)
		{
		  if (added > 0)
		    strcat (query, ",");
		  if (m_binary_data == TRUE)
		    {
		      n = m_field_lengths[i];
		      p = (char *) malloc (n * 2 + 1);
		      nl = mysql_escape_string (p, m_field_values[i], n);
		    }
		  strcat (query, m_fields[i].name);
		  strcat (query, "='");
		  if (m_binary_data == TRUE)
		    strncat (query, p, nl);
		  else
		    strcat (query, m_field_values[i]);
		  strcat (query, "'");
#ifdef DEBUG
		  fprintf (stderr, "Update: partial query='%s'\n", query);
		  fflush (stderr);
#endif
		  if (m_binary_data == TRUE)
		    free (p);
		  added++;
		}
	    }
	}
      if (flag == M_DELETE)
	sprintf (query, "DELETE FROM %s", table);
      strcat (query, " WHERE ");
      added = 0;
      for (i = 0; i < l; i++)
	{
	  if (m_row[i] != NULL)
	    {
	      if (added > 0)
		strcat (query, " AND ");
	      if (m_binary_data == TRUE)
		{
		  n = strlen (m_row[i]);
		  p = (char *) malloc (n * 2 + 1);
		  nl = mysql_escape_string (p, m_row[i], n);
		}
	      strcat (query, m_fields[i].name);
	      strcat (query, "='");
	      if (m_binary_data == TRUE)
		strncat (query, p, nl);
	      else
		strcat (query, m_row[i]);
	      strcat (query, "'");
	      if (m_binary_data == TRUE)
		free (p);
	      added++;
	    }
	}
    }
  else
    {
      tot = 0L;
      added = 0;
      for (i = 0; i < l; i++)
	{
	  if (m_field_values[i] != NULL)
	    {
	      tot += strlen (m_field_values[i]) * 2 + 1;
	      tot += strlen (m_fields[i].name);
	      tot += 5L;
	    }
	}
      tot += 100L;
/*
   printf("Allocating %ld bytes for QUERY\n", tot); fflush(stdout);
 */
      query = (char *) malloc (tot);
      sprintf (query, "INSERT INTO %s SET ", table);
      added = 0;
      for (i = 0; i < l; i++)
	{
	  if (m_field_values[i] != NULL)
	    {
	      if (added > 0)
		strcat (query, ",");
	      if (m_binary_data == TRUE)
		{
		  n = strlen (m_field_values[i]);
		  p = (char *) malloc (n * 2 + 1);
		  nl = mysql_escape_string (p, m_field_values[i], n);
		}
	      strcat (query, m_fields[i].name);
	      strcat (query, "='");
	      if (m_binary_data == TRUE)
		strncat (query, p, nl);
	      else
		strcat (query, m_field_values[i]);
	      strcat (query, "'");
	      if (m_binary_data == TRUE)
		free (p);
	      added++;
	    }
	}
    }
  if (flag == M_DELETE)
    strcat (query, " LIMIT 1");
#ifdef DEBUG
  fprintf (stderr, "QUERY: '%s'\n", query);
  fflush (stderr);
#endif
  if (mysql_query (&mysql, query))
    {
      fprintf (stderr, "ERROR running query: %s\n", mysql_error (&mysql));
      return (EOF);
    }
  free (query);
  return (mysql_affected_rows (&mysql));
}

AddNew ()
{
  int i;

  for (i = 0; i < FieldCount (); i++)
    {
      m_field_values[i] = "";
      m_field_lengths[i] = 0;
    }
  m_add_flag = TRUE;
  m_update_flag = FALSE;
}

int
Edit ()
{
  if (!m_row)
    return (EOF);
  m_add_flag = FALSE;
  m_update_flag = TRUE;
  return (NULL);
}

CloseRecordset ()
{
  if (m_result_valid == TRUE)
    {
      free (m_field_lengths);
      free (m_field_values);
      mysql_free_result (m_result);
    }
  m_result_valid = FALSE;
}

CloseDatabase ()
{
  CloseRecordset ();
  mysql_close (&mysql);
  mysql_valid = FALSE;
}

unsigned int
RecordCount ()
{
  if (m_result_valid == FALSE)
    return (EOF);
  return (mysql_num_rows (m_result));
}

unsigned int
FieldCount ()
{
  if (m_result_valid == FALSE)
    return (EOF);
  return (mysql_num_fields (m_result));
}

unsigned int
AbsolutePosition ()
{
  if (m_result_valid == FALSE)
    return (EOF);
/*
   return (mysql_row_tell (m_result));
 */
  return (m_row_tell + 1);
}

Refresh ()
{
  unsigned int i;

  i = m_row_tell;
  CloseRecordset ();
  OpenRecordset (m_current_query);
  mysql_data_seek (m_result, i);
}

int
RecordsetEOF ()
{
  return (m_EOF);
}

/*
   *
   * auxiliary routines
   *
 */

char *
parse_query (char *lookfor)
{
  static char qs[4096], *ptr, *fptr, *eptr;

  ptr = qs;
  *ptr = NULL;
  eptr = getenv ("QUERY_STRING");
  if (eptr == (char *) NULL)
    return ((char *) NULL);
  if ((fptr = strstr (eptr, lookfor)) == (char *) NULL)
    return ((char *) NULL);
  fptr += strlen (lookfor) + 1;
  while (*fptr != NULL && *fptr != '&')
    {
      *ptr = *fptr;
      ptr++;
      fptr++;
    }
  *ptr = NULL;
  ptr = qs;
  while (*ptr != NULL)
    {
      if (*ptr == '+')
	*ptr = ' ';
      ptr++;
    }
  return (qs);
}
