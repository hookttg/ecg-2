//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <libs/db/classMySql.h>

#include "ecg_main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfmMain *fmMain;
//cMySql __declspec(dllimport) fmysql;
//---------------------------------------------------------------------------
__fastcall TfmMain::TfmMain(TComponent* Owner)
	: TForm(Owner)
    , fmysql(cMySql::getRef())
	{
	fmysql.init();
	}
//---------------------------------------------------------------------------
__fastcall TfmMain::~TfmMain()
	{
	fmysql.shutdown();
	}
//---------------------------------------------------------------------------
void __fastcall TfmMain::FormCreate(TObject *Sender)
	{
	tStartup->Enabled = true;
	}
//---------------------------------------------------------------------------
void __fastcall TfmMain::tStartupTimer(TObject *Sender)
	{
	tStartup->Enabled = false;
	Cursor = crHourGlass;


	//Testschalter NoMySql ber�cksichtigen, erm�glichst einen Testbetrieb ohne MySql
	TIniFile* Ini = new TIniFile(ftools.GetIniFile());
	bNoMySql = Ini->ReadBool("MySql", "NoMySql", false);
	delete Ini;

	setStatus("startup EcgTool...loading MySql-Database");

	/* TODO
	if (ftools.GetComputerBS() != "Windows")
		{
		if (Application->MessageBox(
			ftools.fmt(
				"Es wurde das Betriebssystem '%s' erkannt.\r\n\r\n"
				"Das Programm ist (vorl�ufig) auf ein Windows-Betriebssystem ausgelegt. "
				"Einige Funktionen sind mit anderen Betriebssystemem unter Umst�nden "
				"nicht verf�gbar.\r\n\r\n"
				"Soll das EcgTool-Programm abgebrochen werden?",
				ftools.GetComputerBS()).c_str(),
			"Falsches Betriebssystem", MB_YESNO) == ID_YES)
			{
			Close();
			return;
			}
		}
	setStatus(ftools.GetComputer(), 2);
	*/
	/* TODO
	if (ftools.GetComputerProzessor() != "64Bit")
		{
		Application->MessageBox(
			ftools.fmt(
				"Es wurde der Systemtyp '%s' erkannt.\r\n\r\n"
				"Das Programm ist auf ein 64Bit-Windows-Betriebssystem ausgelegt. "
				"Einige Funktionen sind mit anderen Betriebssystemem unter Umst�nden "
				"nicht verf�gbar, z.B. die Choi-SVM-Klassifizierung.\r\n\r\n",
				ftools.cpData.BSProzessor).c_str(),
			"Warnung 64Bit-System", MB_OK);
		setStatus("## 32Bit-System ggf. nicht ausreichend ##", 2);
		}
	else
		setStatus("64Bit-System  - ok", 2);
	*/

	if (!setupDatabase())
		{
		Close();
		return;
		}

	setStatus("startup EcgTool...reading MySql-Database");
	setDbInfo();

	setStatus("Startup finished - ready to go");

	Cursor = crDefault;
	}
//---------------------------------------------------------------------------
void __fastcall TfmMain::FormClose(TObject *Sender, TCloseAction &Action)
	{
	//
	}
//---------------------------------------------------------------------------
bool TfmMain::setupDatabase()
	{
	if (bNoMySql)
		return true; //simulieren, die DB w�r ge�ffnet worden

	//falls die Datenbank nicht vorhanden ist, anlegen
	setStatus("startup Database...checking existance");
	if (!fmysql.openWithoutDb())
		{
		Application->MessageBox(
			ftools.fmt(
				"Die MySql-Datenbankverbindung konnte nicht initialisiert werden. "
				"Die Funktion meldet: %s", fmysql.error_msg.c_str()
				).c_str(),
			L"Fehler beim �ffnen der Datenbank",
			MB_OK);
		return false;
		}

	setStatus("startup Database...checking existance, MySql found");
	if (!fmysql.dbExists())
		{
		if (Application->MessageBox(
			ftools.fmt(
				"Die MySql-Datenbank 'ecg' ist nicht vorhanden. "
				"Soll sie nun vom Programm erstellt werden ?\n\n"
				"Ansonsten wird das Programm abgebrochen.").c_str(),
			L"Datenbank erstellen ?",
			MB_YESNO) == IDNO)
			{
			fmysql.close();
			return false;
			}

		setStatus("startup Database...creating database");
		if (!fmysql.create())
			{
			Application->MessageBox(
				ftools.fmt(
					"Die MySql-Datenbank 'ecg' konnte nicht initialisiert werden. "
					"Die Funktion meldet: %s", fmysql.error_msg).c_str(),
				L"Fehler beim �ffnen der Datenbank",
				MB_OK);
			return false;
			}
		}

	setStatus("startup Database...checking existance, DB found");
	if (!fmysql.close())
		;

	//Datenbank �ffnen
	setStatus("startup Database...opening database");
	if (!fmysql.open())
		{
		Application->MessageBox(
			ftools.fmt(
				"Die MySql-Datenbank 'ecg' konnte nicht ge�ffnet werden. "
				"Die Funktion meldet: %s", fmysql.error_msg).c_str(),
			L"Fehler beim �ffnen der Datenbank",
			MB_OK);
		return false;
		}

	setStatus("startup Database...ready");
	return true;
	}
//---------------------------------------------------------------------------
void TfmMain::setStatus(String status, int panel) //panel ist vorbesetzt mit 0
	{
	StatusBar->Panels->Items[panel]->Text = status;
	}
//---------------------------------------------------------------------------
void TfmMain::setDbInfo()
	{
    int test = fmysql.people.getSize();
	if (bNoMySql)
		setStatus("### MySql-Datenbank wird nicht verwendet ###", 1);
	else
        //TODO %s wieder in %d umwandeln
		setStatus(
		ftools.fmt("%d Personen, %d Sessions, %d EKG-Datens�tze, %d Features",
		fmysql.people.getSize(),
		fmysql.sessions.getSize(),
		fmysql.ecg.getSize(),
		fmysql.features.getSize()), 1);
	}
//---------------------------------------------------------------------------
void __fastcall TfmMain::FormKeyPress(TObject *Sender, System::WideChar &Key)
	{
	if (Key == VK_ESCAPE)
		{
		Key = 0;
		Close();
        }
	}
//---------------------------------------------------------------------------
//---  Testbuttons  ---------------------------------------------------------
//---------------------------------------------------------------------------
void TfmMain::ln(String line)
	{
	mInfo->Lines->Add(line);
	}
//---------------------------------------------------------------------------
void __fastcall TfmMain::btTestMySqlClick(TObject *Sender)
	{
	cMySqlPeople& pp = fmysql.people;

	ln(ftools.fmt("Datenbank-Test 'People'"));

	ln(ftools.fmt("\tAnz. Personen: %s", String(pp.getSize())));

	sPeople p;
	sprintf(p.firstname, "%s", "Otto");
	sprintf(p.lastname,  "%s", "Mustermann");
	p.sex       = 0;
	p.age       = 99;
	p.height    = 180;
	p.weight    = 80;

	if (pp.insert(p))
		ln(ftools.fmt("\tPerson hinzugef�gt: (%d) %s",
			pp.row.ident, String(pp.row.lastname)));
	else
		{
		ln(ftools.fmt("\t## Fehler, Person konnte nicht gespeichert werden"));
		ln(ftools.fmt("\tMySqlPeople meldet: %s", pp.error_msg));
		}

	int id = pp.row.ident;
	if (pp.get(id))
		ln(ftools.fmt("\tPerson %d geladen: %s %s",
			pp.row.ident,
			String(pp.row.firstname),
			String(pp.row.lastname)));
	else
		{
		ln(ftools.fmt("\t## Fehler, Person konnte nicht geladen werden"));
		ln(ftools.fmt("\tMySqlPeople meldet: %s", pp.error_msg));
		}

	if (pp.deleteByIdent(id))
		ln(ftools.fmt("\tPerson %d gel�scht", id));
	else
		{
		ln(ftools.fmt("\t## Fehler, Person konnte nicht gel�scht werden"));
		ln(ftools.fmt("\tMySqlPeople meldet: %s", pp.error_msg));
		}

	ln("Test-Ende");
	}
//---------------------------------------------------------------------------

