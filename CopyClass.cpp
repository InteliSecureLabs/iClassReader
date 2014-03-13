//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
//---------------------------------------------------------------------------
USEFORM ("uMain.cpp", FM_Main);
//---------------------------------------------------------------------------
WINAPI
_tWinMain (HINSTANCE, HINSTANCE, LPTSTR, int)
{
  try
  {
    Application->Initialize ();
    Application->MainFormOnTaskBar = true;
    Application->Title = "CopyClass";
    Application->CreateForm (__classid (TFM_Main), &FM_Main);
    Application->Run ();
  }
  catch (Exception & exception)
  {
    Application->ShowException (&exception);
  }
  catch ( ...)
  {
    try
    {
      throw Exception ("");
    }
    catch (Exception & exception)
    {
      Application->ShowException (&exception);
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
