//---------------------------------------------------------------------------
#ifndef uMainH
#define uMainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
#include <Menus.hpp>
//---------------------------------------------------------------------------
class TFM_Main:public TForm
{
  __published:			// Von der IDE verwaltete Komponenten
  TGroupBox * GroupBox1;
  TComboBox *CB_Readers;
  TGroupBox *CB_Authenticate;
  TComboBox *ED_AuthKey;
  TRadioGroup *RG_KeyType;
  TStaticText *ST_AuthKey;
  TLabel *Label1;
  TEdit *ED_Serial;
  TLabel *Label2;
  TEdit *ED_ConfigBlock;
  TLabel *Label3;
  TEdit *ED_AppIssuer;
  TBitBtn *BT_CardReload;
  TBitBtn *BT_Auth;
  TGroupBox *GroupBox3;
  TButton *BT_ReadCard;
  TCheckBox *CB_Decrypt;
  TStatusBar *StatusBar;
  TRichEdit *Memo;
  TTimer *Timer;
  TPopupMenu *PopupMenu;
  TMenuItem *BT_CopyToClipboard;
  TComboBox *PN_Select;
	TMemo *Memo1;
  void __fastcall FormKeyDown (TObject * Sender, WORD & Key,
			       TShiftState Shift);
  void __fastcall BT_CardReloadClick (TObject * Sender);
  void __fastcall BT_CardCloseClick (TObject * Sender);
  void __fastcall BT_AuthClick (TObject * Sender);
  void __fastcall ClearStatusPanel (TObject * Sender, TMouseButton Button,
				    TShiftState Shift, int X, int Y);
  void __fastcall BT_ReadCardClick (TObject * Sender);
  void __fastcall MemoKeyPress (TObject * Sender, wchar_t & Key);
  void __fastcall MemoMouseDown (TObject * Sender, TMouseButton Button,
				 TShiftState Shift, int X, int Y);
  void __fastcall BT_CopyToClipboardClick (TObject * Sender);
  void __fastcall PageSelect (int P2);
  void __fastcall CardConfig (UCHAR block);
  void __fastcall CardePurse (UCHAR block);
private:			// Anwender-Deklarationen
  SCARDCONTEXT hContext;
  SCARDHANDLE hCard;
  TStringList *m_PrevLines;
  UnicodeString __fastcall CardSelect (UCHAR p2);
  int __fastcall CardAuth (__int64 key, UCHAR type);
  UnicodeString __fastcall HexDump (PUCHAR data, ULONG len);
  UnicodeString __fastcall CardRead (UCHAR block);

  void __fastcall PrettyPrint (void);
  void __fastcall SetLineNumberColor (int Line, TColor Color);

public:			// Anwender-Deklarationen
    __fastcall TFM_Main (TComponent * Owner);
    __fastcall ~ TFM_Main (void);
};
//---------------------------------------------------------------------------
extern PACKAGE TFM_Main *FM_Main;
//---------------------------------------------------------------------------
#endif
