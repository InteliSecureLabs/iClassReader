//---------------------------------------------------------------------------
#include <vcl.h>
#include <winscard.h>
#pragma hdrstop
//---------------------------------------------------------------------------
#include "des.h"
#include "uMain.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
#pragma link "winscard.lib"
#pragma link "lib/scardsyn.lib"
//---------------------------------------------------------------------------
TFM_Main *FM_Main;
//---------------------------------------------------------------------------
static const UCHAR hid_decryption_key[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static struct memory_t{
   int k16;
   int book;
   int k2;
   int lockauth;
   int keyaccess;
  } memory;

static struct fuse_t{
   bool fpers;
   bool code1;
   bool code0;
   bool crypt1;
   bool crypt0;
   bool fprod1;
   bool fprod0;
   bool ra;
  } fuse;
//---------------------------------------------------------------------------
extern "C" ULONG __stdcall SCardCLICCTransmit (IN SCARDHANDLE ulHandleCard,
					       IN PUCHAR pucSendData,
					       IN ULONG ulSendDataBufLen,
					       IN OUT PUCHAR pucReceivedData,
					       IN OUT PULONG
					       pulReceivedDataBufLen);
//---------------------------------------------------------------------------
__fastcall
TFM_Main::TFM_Main (TComponent * Owner):
TForm (Owner)
{
  int i;
  wchar_t readers[1024], *wc;
  DWORD res, length = sizeof (readers) / sizeof (readers[0]);

  // set up resizing constraints
  Constraints->MinWidth = Width;
  Constraints->MinHeight = Height;

  // init random pool
  randomize ();

  hContext = hCard = NULL;
  m_PrevLines = NULL;

  res = SCardEstablishContext (SCARD_SCOPE_USER, NULL, NULL, &hContext);
  if (res != SCARD_S_SUCCESS)
    ShowMessage ("Can't establish RFID reader context");
  else
    {
      res = SCardListReaders (hContext, NULL, readers, &length);
      if (res != SCARD_S_SUCCESS)
	ShowMessage ("Can't retrieve list of installed readers");
      else
	{
	  wc = readers;
	  while (*wc)
	    {
	      i = CB_Readers->Items->Add (UnicodeString (wc));
	      if (wcsstr (wc, L"-CL"))
		CB_Readers->ItemIndex = i;
	      wc += wcslen (wc) + 1;
	    }
	  if (CB_Readers->Items->Count)
	    {
	      BT_CardReload->Enabled = true;
	      BT_CardReload->Click ();
	      return;
	    }
	  else
	    ShowMessage ("Can't find a single smart card reader");
	}
	}
  Application->Terminate ();
}

//---------------------------------------------------------------------------
__fastcall
TFM_Main::~
TFM_Main (void)
{
  if (hCard)
    SCardReleaseContext (hCard);
  if (hContext)
    SCardReleaseContext (hContext);
  if (m_PrevLines)
    delete m_PrevLines;
}

//---------------------------------------------------------------------------
void __fastcall
TFM_Main::FormKeyDown (TObject * Sender, WORD & Key, TShiftState Shift)
{
  if (Key == VK_F5)
    BT_CardReload->Click ();
}

//---------------------------------------------------------------------------

void __fastcall
TFM_Main::BT_CardReloadClick (TObject * Sender)
{
  DWORD res;
  DWORD dwActiveProtocol;
  SCARD_READERSTATE state;
  bool done;

  if (hCard)
    BT_CardCloseClick (this);

  res = SCardConnect (hContext,
		      CB_Readers->Text.c_str (),
		      SCARD_SHARE_SHARED,
		      SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
		      &hCard, &dwActiveProtocol);

  done = (res == SCARD_S_SUCCESS);

  ED_AuthKey->Enabled = done;
  BT_Auth->Enabled = done;
  RG_KeyType->Enabled = done;

  ED_Serial->Text = done ? CardSelect (0x04) : UnicodeString ();
  ED_ConfigBlock->Text = done ? CardSelect (0x08) : UnicodeString ();
  ED_AppIssuer->Text = done ? CardSelect (0x0C) : UnicodeString ();

  if (done)
    ActiveControl = BT_Auth;
}

//---------------------------------------------------------------------------

void __fastcall
TFM_Main::BT_CardCloseClick (TObject * Sender)
{
  if (hCard)
    {
      SCardReleaseContext (hCard);
      hCard = NULL;
    }
}

//---------------------------------------------------------------------------
UnicodeString __fastcall
TFM_Main::CardSelect (UCHAR p2)
{
  int res, i;
  UnicodeString hex;
  UCHAR ucReceivedData[64] = { 0 };
  ULONG ulNoOfDataReceived = sizeof (ucReceivedData);
  UCHAR data[] = { 0x80, 0xA6, 0x00, p2, p2 ? 0x08 : 0x00 };

  res =
    SCardCLICCTransmit (hCard, data, sizeof (data), ucReceivedData,
			&ulNoOfDataReceived);
  if ((res != SCARD_S_SUCCESS) || (ulNoOfDataReceived < 2))
    ShowMessage ("Error in SCardCLICCTransmit");
  else
    {
      res = ucReceivedData[ulNoOfDataReceived - 2];
      if (res != 0x90)
	return Format ("APDU Error=0x%02X", OPENARRAY (TVarRec, (res)));
      else
	{
	  res = ulNoOfDataReceived - 2;
	  for (i = 0; i < res; i++)
	    hex += IntToHex (ucReceivedData[i], 2);
	  return hex;
	}
    }
  return "Error";
}

//---------------------------------------------------------------------------
void __fastcall
TFM_Main::BT_AuthClick (TObject * Sender)
{
  const wchar_t *p;
  wchar_t nibble;
  __int64 k;

  k = 0;
  p = ED_AuthKey->Text.c_str ();
  while ((nibble = *p++) != 0)
	{
	  if (nibble >= '0' && nibble <= '9')
	nibble -= '0';
	  else if (nibble >= 'a' && nibble <= 'f')
	nibble -= 'a' - 0xA;
	  else if (nibble >= 'A' && nibble <= 'F')
	nibble -= 'A' - 0xA;
	  else
	nibble = 0;

	  k = (k << 4) | nibble;
	}

  ED_AuthKey->Text = IntToHex (k, 16);

  if (RG_KeyType->ItemIndex)
    BT_ReadCard->Enabled = CardAuth (k, RG_KeyType->ItemIndex - 1) > 0;
  else
    BT_ReadCard->Enabled = CardAuth (k, 0) || CardAuth (k, 1);

  if (BT_ReadCard->Enabled)
    ActiveControl = BT_ReadCard;
}
//---------------------------------------------------------------------------
void __fastcall
TFM_Main::PageSelect (int P2){
	int res;
	UCHAR ucReceivedData[64] = { 0 };
	ULONG ulNoOfDataReceived = sizeof (ucReceivedData);
	UCHAR data[7] = { 0x80, 0xa6, 0x01, 0x00, 0x00, 0x00, 0x00 };
	switch(P2){
			case 1:
				data[4]=0x01;
				data[5]=0x01;
				break;
			case 2:
				data[4]=0x01;
				data[5]=0x02;
				break;
		   case	3:
				data[4]=0x01;
				data[5]=0x03;
				break;
		   case	4:
				data[4]=0x01;
				data[5]=0x04;
				break;
		   case	5:
				data[4]=0x01;
				data[5]=0x05;
				break;
		   case	6:
				data[4]=0x01;
				data[5]=0x06;
				break;
		   case	7:
				data[4]=0x01;
				data[5]=0x07;
				break;
		   case	8:
				data[4]=0x01;
				data[5]=0x10;
				break;
		   case	9:
				data[4]=0x01;
				data[5]=0x11;
				break;
		   case	10:
				data[4]=0x01;
				data[5]=0x12;
				break;
		   case	11:
				data[4]=0x01;
				data[5]=0x13;
				break;
		   case	12:
				data[4]=0x01;
				data[5]=0x14;
				break;
		   case	13:
				data[4]=0x01;
				data[5]=0x15;
				break;
		   case	14:
				data[4]=0x01;
				data[5]=0x16;
				break;
		   case	15:
				data[4]=0x01;
				data[5]=0x17;
				break;
			default:
				break;
	}
	if(P2 != 0){
		res = SCardCLICCTransmit (hCard, data, (sizeof (data)-1), ucReceivedData,
			&ulNoOfDataReceived);
	}else{
	  res = SCardCLICCTransmit (hCard, data, sizeof (data), ucReceivedData,
			&ulNoOfDataReceived);
	}
	if ((res != SCARD_S_SUCCESS) || (ulNoOfDataReceived < 2)){
		StatusBar->SimpleText = "Error in Page Select SCardCLICCTransmit";
	}
}
//---------------------------------------------------------------------------
int __fastcall
TFM_Main::CardAuth (__int64 key, UCHAR type)
{
  int res, i;
  UnicodeString hex;
  UCHAR ucReceivedData[64] = { 0 };
  ULONG ulNoOfDataReceived = sizeof (ucReceivedData);
  UCHAR data[13] = { 0x80, 0x82, 0x00, 0xF0, 0x08 };

  for (i = 0; i < 8; i++)
    {
      data[5 + 8 - 1 - i] = (UCHAR) key;
      key >>= 8;
    }

  res =
	SCardCLICCTransmit (hCard, data, sizeof (data), ucReceivedData,
			&ulNoOfDataReceived);
  if ((res != SCARD_S_SUCCESS) || (ulNoOfDataReceived < 2))
	{
	  StatusBar->SimpleText = "Error in LoadKey SCardCLICCTransmit";
	  return 0;
	}

  res = ucReceivedData[ulNoOfDataReceived - 2];
  if (res != 0x90)
    {
      StatusBar->SimpleText = "APDU LoadKey error=0x" + IntToHex (res, 2);
      return 0;
    }

  data[1] = 0x88;
  data[2] = type;

  ulNoOfDataReceived = sizeof (ucReceivedData);
  res =
    SCardCLICCTransmit (hCard, data, 4, ucReceivedData, &ulNoOfDataReceived);
  if ((res != SCARD_S_SUCCESS) || (ulNoOfDataReceived < 2))
    StatusBar->SimpleText = "Error in Authenticate SCardCLICCTransmit";
  else
    {
      res = ucReceivedData[ulNoOfDataReceived - 2];
      if (res != 0x90)
	{
	  res = res << 8 | ucReceivedData[ulNoOfDataReceived - 1];
	  StatusBar->SimpleText =
	    "APDU Authenticate error=0x" + IntToHex (res, 4);
	}
      else
	{
	  if (type && CB_Decrypt->Checked)
		CB_Decrypt->Checked = false;

	  StatusBar->SimpleText =
	    "Authenticated with Key(" + IntToStr (type + 1) + ")";
	  return 1;
	}
    }

  return 0;
}

//---------------------------------------------------------------------------

void __fastcall
TFM_Main::ClearStatusPanel (TObject * Sender, TMouseButton Button,
			    TShiftState Shift, int X, int Y)
{
  StatusBar->SimpleText = "";
}

//---------------------------------------------------------------------------
void __fastcall
TFM_Main::BT_ReadCardClick (TObject * Sender)
{
  int i, sel;
  UnicodeString hex;
  UCHAR P2;
  const wchar_t *q;
  wchar_t nibble;
  __int8 r;

  // remember previous content and clear Memo
  if (!m_PrevLines)
    m_PrevLines = new TStringList ();
  m_PrevLines->Text = Memo->Lines->Text;
  Memo->Clear ();

  r = 0;
  q = PN_Select->Text.c_str();
  while ((nibble = *q++) != 0)
	{
	  if (nibble >= '0' && nibble <= '9')
	nibble -= '0';
	  else if (nibble >= 'a' && nibble <= 'f')
	nibble -= 'a' - 0xA;
	  else if (nibble >= 'A' && nibble <= 'F')
	nibble -= 'A' - 0xA;
	  else
	nibble = 0;

	  r = (r << 4) | nibble;
	}

  PN_Select->Text = IntToHex (r, 2);

  if (PN_Select->Text == "00") {
		  PageSelect(0);
  }else if(PN_Select->Text == "01") {
		  PageSelect(1);
  }else if(PN_Select->Text == "02") {
		  PageSelect(2);
  }else if(PN_Select->Text == "03") {
		  PageSelect(3);
  }else if(PN_Select->Text == "04") {
		  PageSelect(4);
  }else if(PN_Select->Text == "05") {
		  PageSelect(5);
  }else if(PN_Select->Text == "06") {
		  PageSelect(6);
  }else if(PN_Select->Text == "07") {
		  PageSelect(7);
  }else if(PN_Select->Text == "10") {
		  PageSelect(8);
  }else if(PN_Select->Text == "11") {
		  PageSelect(9);
  }else if(PN_Select->Text == "12") {
		  PageSelect(10);
  }else if(PN_Select->Text == "13") {
		  PageSelect(11);
  }else if(PN_Select->Text == "14") {
		  PageSelect(12);
  }else if(PN_Select->Text == "15") {
		  PageSelect(13);
  }else if(PN_Select->Text == "16") {
		  PageSelect(14);
  }else if(PN_Select->Text == "17") {
		  PageSelect(15);
  }else{
		  PageSelect(0);
  }

  CardConfig(1);
  CardePurse(2);

  for (i = 0; i <= 0xFF; i++)
	{
	  hex = CardRead (i);
	  if (hex.IsEmpty ())
	break;
	  else
	{
	  Memo->Lines->Add (IntToHex (i, 2) + ": " + hex);
	  SetLineNumberColor (i, clBlue);
	}
    }

  // set memo active
  ActiveControl = Memo;
}

//---------------------------------------------------------------------------
UnicodeString __fastcall
TFM_Main::HexDump (PUCHAR data, ULONG len)
{
  ULONG t;
  UnicodeString hex = "";

  for (t = 0; t < len; t++)
	hex += (t ? " " : "") + IntToHex (*data++, 2);

  return hex;
}
//---------------------------------------------------------------------------
void __fastcall TFM_Main::CardConfig (UCHAR block){

  int res;
  static int app_limit,block_lock,chip,mem,eas,fuse_b;
  static uint16_t app_otp;
  UCHAR ucReceivedData[64] = { 0 };
  UCHAR compare[64] = { 0 };
  UCHAR ucDecryptedReceivedData[64] = { 0 };
  ULONG ulNoOfDataReceived = sizeof (ucReceivedData);
  UCHAR data[] = { 0x80, 0xB0, 0x00, block, 0x08 };
  UCHAR *output = ucDecryptedReceivedData;
  int outputLength;

  res =
	SCardCLICCTransmit (hCard, data, sizeof (data), ucReceivedData,
			&ulNoOfDataReceived);

  if ((res != SCARD_S_SUCCESS) || (ulNoOfDataReceived < 2)){
	  ShowMessage ("Error in CardRead SCardCLICCTransmit");
  }else{
	  res = ucReceivedData[ulNoOfDataReceived - 2];
	  if (res == 0x90){
		Memo1->Clear ();
		Memo1->Lines->Add("Configuration");
		Memo1->Lines->Add("-----------------");
		outputLength = ulNoOfDataReceived - 2;
		app_limit=(int)ucReceivedData[0];
		app_otp=ucReceivedData[1]|(ucReceivedData[2] << 8);
		block_lock=(int)ucReceivedData[3];
		chip=(int)ucReceivedData[4];
		mem=(int)ucReceivedData[5];
		eas=(int)ucReceivedData[6];
		fuse_b=(int)ucReceivedData[7];
		//-------------------------
		//ShowMessage(mem);
		memory.k16=       (mem & 0x80);
		memory.book=      (mem & 0x20);
		memory.k2 =       (mem & 0x8);
		memory.lockauth = (mem & 0x2);
		memory.keyaccess= (mem & 0x1);
		//------------------------
		fuse.fpers=     (fuse_b & 0x80);
		fuse.code1=     (fuse_b & 0x40);
		fuse.code0=     (fuse_b & 0x20);
		fuse.crypt1 =   (fuse_b & 0x10);
		fuse.crypt0 =   (fuse_b & 0x8);
		fuse.fprod1 =   (fuse_b & 0x4);
		fuse.fprod0 =   (fuse_b & 0x2);
		fuse.ra     =   (fuse_b & 0x1);

	  }//0x90
  }
  if (memory.k16) {
	 Memo1->Lines->Add("16K Enabled. ");
  }
  if (memory.k2) {
	 Memo1->Lines->Add("2K Enabled. ");
  }
  if (memory.book) {
	 Memo1->Lines->Add("Book Enabled. ");
  }
  if (memory.book) {
	 Memo1->Lines->Add("Lock Auth Enabled. ");
  }
  if (memory.book) {
	 Memo1->Lines->Add("KeyAccess:");
	 Memo1->Lines->Add("\tRead A - Kd");
	 Memo1->Lines->Add("\tRead B - Kc");
	 Memo1->Lines->Add("\tWrite A - Kd");
	 Memo1->Lines->Add("\tWrite B - Kc");
	 Memo1->Lines->Add("\tDebit  - Kd or Kc");
	 Memo1->Lines->Add("\tCredit - Kc");
  } else{
	 Memo1->Lines->Add(" KeyAccess:");
	 Memo1->Lines->Add("\tRead A - Kd or Kc");
	 Memo1->Lines->Add("\tRead B - Kd or Kc");
	 Memo1->Lines->Add("\tWrite A - Kc");
	 Memo1->Lines->Add("\tWrite B - Kc");
	 Memo1->Lines->Add("\tDebit  - Kd or Kc");
	 Memo1->Lines->Add("\tCredit - Kc");
  }
  if (fuse.fpers) {
	 Memo1->Lines->Add("Personalisation");
  }  else  Memo1->Lines->Add("Application Mode");
  if (fuse.crypt1 && fuse.crypt0) {
	 Memo1->Lines->Add("Secure Mode");
  }else{
	 if ((fuse.crypt1 ==1)&& (fuse.crypt0 ==0)){
	   Memo1->Lines->Add("Secured Page Key Values Locked");
	 }
	 if ((fuse.crypt1 ==0)&& (fuse.crypt0 ==1)){
	   Memo1->Lines->Add("Non Secured Page");
	 }
	 if ((fuse.crypt1 ==0)&& (fuse.crypt0 ==0)&& (fuse.ra==1)){
	   Memo1->Lines->Add("No authentication - Chip READ ONLY");
	 }
	 if ((fuse.crypt1 ==0)&& (fuse.crypt0 ==0)&& (fuse.ra==0)){
	   Memo1->Lines->Add("No authentication - R/W Forbidden accpet Blocks 0 + 1");
	 }
  }
  ActiveControl = Memo1;
}
void __fastcall TFM_Main::CardePurse (UCHAR block){

  int res;
  static uint16_t s1_charge, s1_recharge, s2_charge, s2_recharge;
  UCHAR ucReceivedData[64] = { 0 };
  UCHAR compare[64] = { 0 };
  UCHAR ucDecryptedReceivedData[64] = { 0 };
  ULONG ulNoOfDataReceived = sizeof (ucReceivedData);
  UCHAR data[] = { 0x80, 0xB0, 0x00, block, 0x08 };
  UCHAR *output = ucDecryptedReceivedData;
  int outputLength;

  res =
	SCardCLICCTransmit (hCard, data, sizeof (data), ucReceivedData,
			&ulNoOfDataReceived);

  if ((res != SCARD_S_SUCCESS) || (ulNoOfDataReceived < 2)){
	  ShowMessage ("Error in CardRead SCardCLICCTransmit");
  }else{
	  res = ucReceivedData[ulNoOfDataReceived - 2];
	  if (res == 0x90){
		Memo1->Lines->Add("");
		Memo1->Lines->Add("ePurse");
		Memo1->Lines->Add("-----------");
		outputLength = ulNoOfDataReceived - 2;
		s1_charge  =ucReceivedData[0]|(ucReceivedData[1] << 8);
		s1_recharge=ucReceivedData[2]|(ucReceivedData[3] << 8);
		s2_charge  =ucReceivedData[4]|(ucReceivedData[5] << 8);
		s2_recharge=ucReceivedData[6]|(ucReceivedData[7] << 8);
	  }
  }

  if ((s1_charge == 65535)&&(s1_recharge ==65535)) {
	  Memo1->Lines->Add("Stage 2");
	  Memo1->Lines->Add("Charge Value: ");
	  Memo1->Lines->Append(s2_charge);
	  Memo1->Lines->Add("Recharge Value: ");
	  Memo1->Lines->Append(s2_recharge);
  }
  if ((s2_charge == 65535)&&(s2_recharge ==65535)) {
	  Memo1->Lines->Add("Stage 1");
	  Memo1->Lines->Add("Charge Value: ");
	  Memo1->Lines->Append(s1_charge);
	  Memo1->Lines->Add("Recharge Value: ");
	  Memo1->Lines->Append(s1_recharge);
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall
TFM_Main::CardRead (UCHAR block)
{
  int res;
  static int app_limit,block_lock,chip,mem,eas,fuse_b;
  static uint16_t app_otp;
  UCHAR ucReceivedData[64] = { 0 };
  UCHAR compare[64] = { 0 };
  UCHAR ucDecryptedReceivedData[64] = { 0 };
  ULONG ulNoOfDataReceived = sizeof (ucReceivedData);
  UCHAR data[] = { 0x80, 0xB0, 0x00, block, 0x08 };
  UCHAR *output = ucDecryptedReceivedData;
  int outputLength;
  int block_cap=255;

  res =
	SCardCLICCTransmit (hCard, data, sizeof (data), ucReceivedData,
			&ulNoOfDataReceived);

  if ((res != SCARD_S_SUCCESS) || (ulNoOfDataReceived < 2)){
	  ShowMessage ("Error in CardRead SCardCLICCTransmit");
	  return "";
  }else{
	  res = ucReceivedData[ulNoOfDataReceived - 2];
	  if (res == 0x6A) return "";

	  if (res != 0x90) return Format ("APDU CardRead Error=0x%02X",OPENARRAY (TVarRec, (res)));
	  else{
		outputLength = ulNoOfDataReceived - 2;
		if (block==1) {
		  app_limit=(int)ucReceivedData[0];
		  app_otp=(int)ucReceivedData[1];
		  block_lock=(int)ucReceivedData[3];
		  chip=(int)ucReceivedData[4];
		  mem=(int)ucReceivedData[5];
		  eas=(int)ucReceivedData[6];
		  fuse_b=(int)ucReceivedData[7];
		}
		if ((memory.k16)) {
						block_cap=255;
		}else block_cap=31;
		if(block<=31){
		  //if (CB_Decrypt->Checked && (block >= 7) && (block <= 9)){
		  if (((ucReceivedData[0]==255)&&(ucReceivedData[1]==255)&&(ucReceivedData[2]==255)&&(ucReceivedData[3]==255))&& block>5) {
			 return HexDump (ucReceivedData, outputLength);
		  }else{
			if (block>5) {
			decrypt_3des (hid_decryption_key,
				ucReceivedData,
				ulNoOfDataReceived, &output, &outputLength);

			return HexDump (output, outputLength);
			} else  return HexDump (ucReceivedData, outputLength);
		  }
	   }
	 }
	}
}

//---------------------------------------------------------------------------


void __fastcall
TFM_Main::MemoKeyPress (TObject * Sender, wchar_t & Key)
{
  if (Key >= ' ' && Key < 0x100)
    {
      if (Key >= 'a' && Key <= 'f')
	Key = (Key - 'a') + 'A';
      else
	if (!((Key >= 'A' && Key <= 'F') ||
	      (Key >= '0') && (Key <= '9') || (Key == ' ') || (Key == ':')))
	Key = NULL;
    }
}

//---------------------------------------------------------------------------
void __fastcall
TFM_Main::MemoMouseDown (TObject * Sender, TMouseButton Button,
			 TShiftState Shift, int X, int Y)
{
  Caption = Memo->ActiveLineNo;

}

//---------------------------------------------------------------------------
void __fastcall
TFM_Main::SetLineNumberColor (int Line, TColor Color)
{
  if (Line < Memo->Lines->Count)
    {
      Memo->SelStart = SendMessage (Memo->Handle, EM_LINEINDEX, Line, 0);
      Memo->SelLength = 3;
      Memo->SelAttributes->Color = Color;
      Memo->SelLength = 0;
    }
}

//---------------------------------------------------------------------------
void __fastcall
TFM_Main::PrettyPrint (void)
{
  int lines, i, j, sel, len, sellen, selstart;
  UnicodeString prev, curr;
  wchar_t *c, *p;

  lines = Memo->Lines->Count;

  for (i = 0; i < lines; i++)
    {
      prev = m_PrevLines->Strings[i];
      curr = Memo->Lines->Strings[i];

      len = curr.Length ();
      if (prev.Length () < len)
	len = prev.Length ();

      c = curr.c_str ();
      p = prev.c_str ();

      sellen = selstart = 0;
	  for (j = 0; j <= len; j++)
	{
	  if ((j < len) && (*c++ != *p++))
	    {
	      if (!sellen)
		selstart = j;
	      sellen++;
	    }
	  else if (sellen)
	    {
	      Memo->SelStart = sel + selstart;
	      Memo->SelLength = sellen;
	      Memo->SelAttributes->Color = clRed;
	    }
	}
    }
  Memo->SelLength = 0;
}

//---------------------------------------------------------------------------

void __fastcall
TFM_Main::BT_CopyToClipboardClick (TObject * Sender)
{
  bool deselected = Memo->SelLength == 0;

  if (deselected)
    Memo->SelectAll ();

  Memo->CopyToClipboard ();

  if (deselected)
    Memo->SelLength = 0;
}

//---------------------------------------------------------------------------


