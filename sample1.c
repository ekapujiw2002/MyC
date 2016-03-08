#include "database.h"
main ()
{
  int i;

  OpenDatabase ("mysql", NULL, NULL, NULL);
  OpenRecordset ("select * from test2");
  while (RecordsetEOF () != EOF)
    {
      for (i = 0; i < FieldCount (); i++)
	printf ("%s\t", GetFieldN (i));
      puts ("");
      MoveNext ();
    }
  CloseRecordset ();
  CloseDatabase ();
  exit (0);
}
