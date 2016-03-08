#include "database.h"
main ()
{
  int i;
  char buf[10], buf2[5];

  OpenDatabase ("mysql", NULL, NULL, NULL);
  OpenRecordset ("delete from test");
  CloseRecordset ();
/*
   * populate the table
 */
  OpenRecordset ("select * from test");
  for (i = 0; i < 5; i++)
    {
      AddNew ();
      sprintf (buf, "item %d", i);
      sprintf (buf2, "%d", i);
      SetField ("name", buf);
      SetField ("num", buf2);
      Update ("test");
    }
/*
   * make the current result set match what's in the database
 */
  Refresh ();
  MoveFirst ();

  while (RecordsetEOF () != EOF)
    {
      for (i = 0; i < FieldCount (); i++)
	printf ("%s\t", GetFieldN (i));
      puts ("");
      MoveNext ();
    }
  MoveFirst ();
/*
   * change "item 3" in the database to "item 5".
   * delete "item 4" from the database
 */
  while (RecordsetEOF () != EOF)
    {
      printf ("Looking at '%s'\n", GetField ("name"));
      if (strcmp (GetField ("name"), "item 3") == 0)
	{
	  puts ("Changing data");
	  Edit ();
	  SetField ("name", "item 5");
	  Update ("test");
	}
      if (strcmp (GetField ("name"), "item 4") == 0)
	{
	  puts ("Deleting data");
	  Delete ("test");
	}
      MoveNext ();
    }
  CloseRecordset ();
  CloseDatabase ();
  exit (0);
}
