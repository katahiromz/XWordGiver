// MShareware.h -- The shareware maker for Win32
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string> // for std::string and std::wstring

////////////////////////////////////////////////////////////////////////////
// useful functions

LPTSTR SwLoadStringDx1(HINSTANCE hInstance, UINT uID);
LPTSTR SwLoadStringDx2(HINSTANCE hInstance, UINT uID);
void SwCenterDialog(HWND hwnd);
int SwCenterMessageBox(HWND hwndParent, LPCTSTR pszText, LPCTSTR pszCaption, UINT uMB_);

////////////////////////////////////////////////////////////////////////////
// MShareware class

class MShareware {
public:
	// NOTE: pszAppKey is the path of the registry key of the application.
	//	   For example, "SOFTWARE\\MyCompany\\MyApp".
	// NOTE: dwTrialDays is the trial period in days.
	// NOTE: salt is the salt string.
	// NOTE: new_version is the current version of this software.
	MShareware(LPCTSTR pszAppKey,
			   const char *pszSha256HashHexString,
			   DWORD dwTrialDays = 15,
			   const char *salt = "",
			   const char *new_version = "");
	MShareware(LPCTSTR pszAppKey,
			   const BYTE *pbHash32Bytes,
			   DWORD dwTrialDays = 15,
			   const char *salt = "",
			   const char *new_version = "");
	virtual ~MShareware();

	// NOTE: MShareware::Start must be called on start-up of the MShareware shareware.
	// NOTE: MShareware::Start returns false if the application cannot be used.
	virtual bool Start(HWND hwndParent = nullptr);

	bool IsRegistered() const;
	bool IsInTrial() const;
	bool IsOutOfTrial() const;

	DWORD GetTrialDays() const;
	bool IsPasswordValid(const char *pszPassword) const;

	// NOTE: UrgeRegister show a dialog and returns true if registered.
	virtual bool UrgeRegister(HWND hwndParent = nullptr);
	bool RegisterPassword(HWND hwndParent, const char *pszPassword);
	// NOTE: Unregister removes the license. It returns true on success.
	//       It never starts a new trial: the status afterwards follows the
	//       existing trial data (IN_TRIAL if time remains, otherwise OUT_OF_TRIAL).
	virtual bool Unregister(HWND hwndParent = nullptr, BOOL bForce = FALSE);
	virtual void ShowErrorMessage(HWND hwndParent, UINT uStringID);
	virtual void ThisCommandRequiresRegistering(HWND hwndParent);
	bool CheckDate();

	virtual int CompareVersion(const char *old_ver, const char *new_ver);

public:
	HINSTANCE		   m_hInstance;
	DWORDLONG		   m_dwlTotalMinutesRemains = 0;
	DWORD			   m_dwTrialDays;
	FILETIME			m_ftStart = { 0, 0 };
	FILETIME			m_ftLastUse = { 0, 0 };	// latest time ever observed (clock-rollback protection)
	bool				m_bClockRollback = false;	// the system clock was set back

	enum SHAREWARE_STATUS {
		IN_TRIAL_FIRST_TIME, IN_TRIAL, OUT_OF_TRIAL, REGD
	};
	SHAREWARE_STATUS	m_status;

public:
	#ifdef UNICODE
		typedef std::wstring tstring;
	#else
		typedef std::string tstring;
	#endif

protected:
	tstring			 m_strAppKey;
	std::string		 m_strSha256HashHexString;
	std::string		 m_strSalt;
	std::string		 m_strNewVersion;
	std::string		 m_strOldVersion;

	DWORD			   m_dwFailedAttempts = 0;

	enum LOAD_RESULT { LOAD_NONE, LOAD_OK, LOAD_TAMPERED };
	enum DATE_RESULT { DATE_OK, DATE_EXPIRED, DATE_ROLLBACK };

	bool CheckRegistry(HWND hwndParent);
	bool CheckTrial(HWND hwndParent);
	int  CheckDateInternal();

	// trial state (HMAC-protected, stored in multiple places)
	int  LoadTrialState();
	bool SaveTrialState();
	bool ImportLegacyTrial(const FILETIME& ftNow);

	// license (HMAC-protected, stored in multiple places)
	bool LoadLicense();
	bool SaveLicense();
	bool MigrateLegacyLicense();

	bool DeleteLegacyValues(bool bTrial, bool bLicense);
	bool RemoveLicense();

	// Override GetMacSecret to add your own build-specific secret.
	virtual void GetMacSecret(std::string& secret) const;
	std::string MakeMacKey(char kind) const;

	DWORD GetUserCheckSum() const;
	virtual void OnTrialFirstTime(HWND hwndParent);
	virtual void OnTrial(HWND hwndParent);
	virtual bool OnOutOfTrial(HWND hwndParent);
	virtual void EncodePassword(void *pass, DWORD size) const;
	virtual void DecodePassword(void *pass, DWORD size) const;

private:
	MShareware();
	// NOTE: MShareware is not copyable.
	MShareware(const MShareware&) = delete;
	MShareware& operator=(const MShareware&) = delete;
};

////////////////////////////////////////////////////////////////////////////
