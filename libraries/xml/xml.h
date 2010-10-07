#ifndef _XML_H
#define _XML_H

// xml.h
// v.0x158
// revision 28 - 11 - 2009
#define XML_VERSION 0x158
#define XML_VERSION_REVISION_DATE "28-11-2009"


// #define ALLOW_SINGLE_QUOTE_VARIABLES
// Define the above tow allow var='x' instead of var="x"

#if defined(__unix) || defined(__APPLE__)
#define LINUX
#endif


#ifdef __BORLANDC__
#pragma warn -pck
#endif

#ifdef _MSC_VER
#define _USERENTRY __cdecl
#endif

#ifdef LINUX
#define _USERENTRY
#define __cdecl
#endif

#ifdef __WATCOMC__
#define _USERENTRY
#endif



// ANSI includes
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#ifdef LINUX
#include <wchar.h>
#endif

#ifdef _WIN32
#ifndef __SYMBIAN32__
#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <tchar.h>
#endif
#endif

#ifdef __SYMBIAN32__
#define _USERENTRY
#define strcmpi strcmp
#include <unistd.h>
#endif


#ifdef XML_USE_STL
#include <vector>
#include <string>
#include <algorithm>
using namespace std;
#endif

#ifndef _Z_H
#define _Z_H
// Z template class
template <class T>class Z
	{
	private:

		T* d;
		size_t ss;

	public:

		Z(size_t s = 0)
			{
			if (s == 0)
				s = 1;
			d = new T[s];
			memset(d,0,s*sizeof(T));
			ss = s;
			}
		~Z()
			{
			delete[] d;
			}

		operator T*()
			{
			return d;
			}

		size_t bs()
			{
			return ss*sizeof(T);
			}

		size_t is()
			{
			return ss;
			}

		void _clear()
			{
			memset(d,0,ss*sizeof(T));
			}

		void Resize(size_t news)
			{
			if (news == ss)
				return; // same size

			// Create buffer to store existing data
			T* newd = new T[news];
			size_t newbs = news*sizeof(T);
			memset((void*)newd,0, newbs);

			if (ss < news)
				// we created a larger data structure
				memcpy((void*)newd,d,ss*sizeof(T));
			else
				// we created a smaller data structure
				memcpy((void*)newd,d,news*sizeof(T));
			delete[] d;
			d = newd;
			ss = news;
			}

		void AddResize(size_t More)
			{
			Resize(ss + More);
			}

	};
#endif // Z_H

#ifdef XML_USE_NAMESPACE
namespace XMLPP
	{
#endif


class XMLHeader;
class XMLElement;
class XMLVariable;
class XMLComment;
class XMLContent;
class XMLCData;
class XML;

typedef struct
	{
	int VersionHigh;
	int VersionLow;
	char RDate[20];
	} XML_VERSION_INFO;


#ifdef _WIN32
struct IMPORTDBTABLEDATA
	{
	char name[256];
	char itemname[100];
	int nVariables;
	char** Variables;
	char** ReplaceVariables;
	};

struct IMPORTDBPARAMS
	{
	char* dbname;
	char* provstr;
	int nTables;
	IMPORTDBTABLEDATA* Tables;
	};
#endif

struct XMLEXPORTFORMAT
	{
	bool UseSpace;
	int nId;
	bool ElementsNoBreak;
	};

#ifdef _WIN32
struct IMPORTRKEYDATA
	{
	HKEY pK;
	int StorageType; // 0 - Native
	// 1 - Registry key from native XML
	// 2 - Registry key from registry XML
	};
#endif



// UNLOAD elements
#ifdef XML_USE_STL
#else
struct XMLUNLOADELEMENT
	{
	int i;
	char* fn[300];
	};
#endif

// Enumerations
enum XML_LOAD_MODE
	{
	XML_LOAD_MODE_LOCAL_FILE = 0,
	XML_LOAD_MODE_MEMORY_BUFFER = 1,
	XML_LOAD_MODE_URL = 2,
	XML_LOAD_MODE_LOCAL_FILE_U = 7,
	};

enum XML_PARSE_STATUS
	{
	XML_PARSE_OK = 0,
	XML_PARSE_NO_HEADER = 1,
	XML_PARSE_ERROR = 2,
	};

enum XML_SAVE_MODE
	{
	XML_SAVE_MODE_ZERO = 0,
	XML_SAVE_MODE_DEFAULT = 1,
	};

enum XML_TARGET_MODE
	{
	XML_TARGET_MODE_FILE = 0,
	XML_TARGET_MODE_MEMORY = 1,
	XML_TARGET_MODE_REGISTRYKEY = 2,
	XML_TARGET_MODE_UTF16FILE = 3,
	};

// Global functions

class XMLHeader
	{
	public:

		// constructors/destructor
		XMLHeader(const char* ht = 0);
		operator const char*();
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();
#ifdef XML_USE_STL
		int Compare(XMLHeader&);
#else
		int Compare(XMLHeader*);
#endif
		void SetEncoding(const char*);
		XMLHeader* Duplicate();


		XMLHeader(XMLHeader&);
		XMLHeader& operator =(XMLHeader&);
		~XMLHeader();

		// XMLComment
#ifdef XML_USE_STL
		vector<XMLComment>& GetComments();
		unsigned int GetCommentsNum();
		XMLComment& AddComment(const char*,int pos = -1);
		XMLComment& AddComment(const XMLComment&,int pos = -1);
		int RemoveComment(unsigned int i);
		int RemoveAllComments();
#else
		XMLComment** GetComments();
		unsigned int GetCommentsNum();
		int AddComment(XMLComment*,int pos);
		int RemoveComment(unsigned int i);
		int RemoveAllComments();
		int SpaceForComment(unsigned int);
#endif
		void Export(FILE* fp,int HeaderMode,XML_TARGET_MODE TargetMode = XML_TARGET_MODE_FILE,class XMLTransform* eclass = 0,class XMLTransformData* edata = 0);

#ifdef XML_USE_STL
		string& GetHeader() {return hdr;}
#endif

	private:

		void Clear();
#ifdef XML_USE_STL
		string hdr;
		vector<XMLComment> comments;
#else
		int TotalCommentPointersAvailable;
		char* hdr;
		unsigned int commentsnum;
		XMLComment** comments;
#endif
	};



#ifdef LINUX
typedef int (*fcmp) (const void *, const void *);
#endif

#ifdef XML_USE_STL
#else
struct XMLBORROWELEMENT
	{
	bool Active;
	class XMLElement* x;
	};
#endif

class XMLElement
	{
	public:

		// constructors/destructor
		XMLElement(XMLElement* par = 0,const char* el = 0,int Type = 0,bool Temp = false);

		//XMLElement& operator =(XMLElement&);
		~XMLElement();

		void Clear();


#ifdef XML_USE_STL
		// STL Functions
		XMLElement& operator[](int);
		XMLElement& AddElement(const char*,int p = -1,bool Temp = false);
		XMLElement& AddElement(const XMLElement&,int p = -1);
#else
		// No STL Functions
		XMLElement* operator[](int);
		XMLElement* AddElement(XMLElement*);
		XMLElement* AddElement(const char*);
		XMLElement* InsertElement(unsigned int,XMLElement*);
#endif


		void SetElementParam(unsigned long long p);
		unsigned long long GetElementParam();
		void Reparse(const char*el,int Type = 0);
		int GetDeep();

#ifdef XML_USE_STL
		int RemoveElementAndKeep(unsigned int i,XMLElement* el);
#else
		int BorrowElement(XMLElement*,unsigned int = (unsigned)-1);
		int ReleaseBorrowedElements();
		int RemoveElementAndKeep(unsigned int i,XMLElement** el);
#endif

		int UpdateElement(XMLElement*,bool UpdateVariableValues = false);
		int FindElement(XMLElement*);
		int FindElement(const char* n);
		XMLElement* FindElementZ(XMLElement*);
		XMLElement* FindElementZ(const char* n,bool ForceCreate = false,char* el = 0,bool Temp = false);
		int RemoveElement(unsigned int i);
		bool EncryptElement(unsigned int i,char* pwd);
		bool DecryptElement(unsigned int i,char* pwd);
		int RemoveElement(XMLElement*);
		int RemoveAllElements();
		int RemoveTemporalElements(bool Deep = false);
		int DeleteUnloadedElementFile(int i);

#ifdef XML_USE_STL
#else
		int UnloadElement(unsigned int i);
		int ReloadElement(unsigned int i);
		int ReloadAllElements();
#endif
		XMLElement* MoveElement(unsigned int i,unsigned int y);


#ifdef XML_USE_STL
		void SortElements();
		void SortVariables();
		bool operator <(const XMLElement&);
#else
#ifdef LINUX
		void SortElements(fcmp);
		void SortVariables(fcmp);
		friend int XMLElementfcmp(const void *, const void *);
		friend int XMLVariablefcmp(const void *, const void *);
#else
		void SortElements(int (_USERENTRY *fcmp)(const void *, const void *));
		void SortVariables(int (_USERENTRY *fcmp)(const void *, const void *));
		friend int _USERENTRY XMLElementfcmp(const void *, const void *);
		friend int _USERENTRY XMLVariablefcmp(const void *, const void *);
#endif
#endif

		XMLElement* Duplicate(XMLElement* = 0);
		XMLElement* Encrypt(const char* pwd);
		XMLElement* Decrypt(const char* pwd);
		void Copy();
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();
#ifdef XML_USE_STL
		int Compare(XMLElement&);
#else
		int Compare(XMLElement*);
#endif

		// XMLComment
#ifdef XML_USE_STL
		vector<XMLComment>& GetComments();
		XMLComment& AddComment(const char*,int);
		XMLComment& AddComment(const XMLComment&,int pos);
#else
		XMLComment** GetComments();
		int AddComment(XMLComment*,int InsertBeforeElement);
		int AddComment(const char*,int);
#endif
		unsigned int GetCommentsNum();
		int RemoveComment(unsigned int i);
		int RemoveAllComments();

		// XMLCData
#ifdef XML_USE_STL
		vector<XMLCData>& GetCDatas();
		XMLCData& AddCData(const char*,int);
		XMLCData& AddCData(const XMLCData&,int);
#else
		XMLCData** GetCDatas();
		int AddCData(XMLCData*,int InsertBeforeElement);
		int AddCData(const char*,int);
#endif
		unsigned int GetCDatasNum();
		int RemoveCData(unsigned int i);
		int RemoveAllCDatas();

		// Content Stuff
#ifdef XML_USE_STL
		vector<XMLContent>& GetContents();
		XMLContent& AddContent(const char*,int,int BinarySize = 0);
		XMLContent& AddContent(const XMLContent&);
#else
		XMLContent** GetContents();
		int AddContent(XMLContent* v,int InsertBeforeElement);
		int AddContent(const char*,int,int BinarySize = 0);
#endif
		int RemoveContent(unsigned int i);
		void RemoveAllContents();
		unsigned int GetContentsNum();

		// Children Stuff
#ifdef XML_USE_STL
		vector<XMLElement>& GetChildren();
#else
		XMLElement** GetChildren();
#endif
		unsigned int GetChildrenNum();
		unsigned int GetAllChildren(XMLElement**,unsigned int deep = 0xFFFFFFFF);
		unsigned int GetAllChildrenNum(unsigned int deep = 0xFFFFFFFF);


		// Variable Stuff
#ifdef XML_USE_STL
		int RemoveVariableAndKeep(unsigned int i,XMLVariable* vr);
		vector<XMLVariable>& GetVariables();
		XMLVariable& AddVariable(const XMLVariable&,int p = -1);
		XMLVariable& AddVariable(const char*,const char*,int p = -1,bool Temp = false);
#ifdef XML_OPTIONAL_MIME
		XMLVariable& AddBinaryVariable(const char*,const char*,int);
#endif
#else
		int AddVariable(XMLVariable*);
		int RemoveVariableAndKeep(unsigned int i,XMLVariable** vr);
		XMLVariable** GetVariables();
		int AddVariable(const char*,const char*);
#ifdef XML_OPTIONAL_MIME
		int AddBinaryVariable(const char*,const char*,int);
#endif
#endif

		int FindVariable(XMLVariable*);
		int FindVariable(const char*  x);
		XMLVariable* FindVariableZ(XMLVariable*);
		XMLVariable* FindVariableZ(const char* x,bool ForceCreate = false,char* defnew = 0,bool Temp = false);
		int RemoveVariable(unsigned int i);
		int RemoveVariable(XMLVariable*);
		int RemoveAllVariables();
		int RemoveTemporalVariables(bool Deep = false);
		unsigned int GetVariableNum();



		XMLElement* GetElementInSection(const char*);
		int XMLQuery(const char* expression,XMLElement** rv,unsigned int deep = 0xFFFFFFFF);
		XMLElement* GetParent();
		void Export(FILE* fp,int ShowAll,XML_SAVE_MODE SaveMode,XML_TARGET_MODE TargetMode = XML_TARGET_MODE_FILE,XMLHeader* hdr = 0,class XMLTransform* eclass = 0,class XMLTransformData* edata = 0);
		void SetExportFormatting(XMLEXPORTFORMAT* xf);
		void SetElementName(const char*);
		size_t GetElementName(char*,int NoDecode = 0);
		size_t GetElementFullName(char*,int NoDecode = 0);
		size_t GetElementUniqueString(char*);
		void SetTemporal(bool);
		bool IsTemporal();
		int   GetType();
		static void Write16String(FILE* fp,const char* s);
#ifdef XML_USE_STL
		XMLElement(const XMLElement&);
		XMLElement(const XMLElement*);
		XMLElement& operator =(const XMLElement&);
		XMLElement& operator =(const XMLElement*);
#endif

	private:

#ifndef XML_USE_STL
		XMLElement(const XMLElement&);
#endif

		unsigned long long param;
		int type; // type, 0 element
		XMLElement* parent; // one

#ifdef XML_USE_STL
		string el; // element name
		vector<XMLElement> children; // many
		vector<XMLVariable> variables; // many
		vector<XMLComment> comments; // many
		vector<XMLContent> contents; // many;
		vector<XMLCData> cdatas; // many
#else
		char* el; // element name
		XMLElement** children; // many
		XMLVariable** variables; // many
		XMLComment** comments; // many
		XMLContent** contents; // many;
		XMLCData** cdatas;
		unsigned int childrennum;
		unsigned int variablesnum;
		unsigned int commentsnum;
		unsigned int contentsnum;
		unsigned int cdatasnum;
		int SpaceForElement(unsigned int);
		int SpaceForVariable(unsigned int);
		int SpaceForComment(unsigned int);
		int SpaceForContent(unsigned int);
		int SpaceForCData(unsigned int);
		int TotalChildPointersAvailable;
		int TotalVariablePointersAvailable;
		int TotalCommentPointersAvailable;
		int TotalContentPointersAvailable;
		int TotalCDataPointersAvailable;
#endif


		bool Temporal;
#ifdef XML_USE_STL
#else
		Z<XMLBORROWELEMENT> BorrowedElements;
		unsigned int NumBorrowedElements;
#endif

		XMLEXPORTFORMAT xfformat;
		static void printc(FILE* fp,XMLElement* root,int deep,int ShowAll,XML_SAVE_MODE SaveMode,XML_TARGET_MODE TargetMode);
		void SetParent(XMLElement*);


	};


class XMLVariable
	{
	public:

		XMLVariable(const char* = 0,const char* = 0,int NoDecode = 0,bool Temp = false);
		~XMLVariable();
		XMLVariable(const XMLVariable&);
		XMLVariable& operator =(const XMLVariable&);


		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();
#ifdef XML_USE_STL
		bool operator <(const XMLVariable&);
		int Compare(XMLVariable&);
#else
		int Compare(XMLVariable*);
#endif

		XMLElement* SetOwnerElement(XMLElement*);
		size_t GetName(char*,int NoDecode = 0) const;
		size_t GetValue(char*,int NoDecode = 0) const;
		int GetValueInt();
		unsigned int GetValueUInt();
		long long GetValueInt64();
		unsigned long long GetValueUInt64();
		float GetValueFloat();
		void SetName(const char*,int NoDecode = 0);
		void SetValue(const char*,int NoDecode = 0);
		void SetValueUInt(unsigned int);
		void SetValueInt(int);
		void SetValueInt64(long long);
		void SetValueUInt64(unsigned long long);
		void SetValueFloat(float);
		void SetFormattedValue(const char* fmt,...);
		template <typename T> T GetFormattedValue(const char* fmt)
			{
			size_t p = GetValue(0);
			Z<char> d(p + 10);
			GetValue(d);
			T x;
			sscanf(d,fmt,&x);
			return x;
			}
		template <typename T> void SetValueX(T t,const char* fmt);
		template <typename T> T GetValueX(const char* fmt);
		XMLVariable* Duplicate();
		void Copy();
		XMLElement* GetOwnerElement();
		void SetTemporal(bool);
		bool IsTemporal();

#ifdef XML_OPTIONAL_MIME
		size_t GetBinaryValue(char*);
		size_t SetBinaryValue(char*,int);
#endif

	private:

		void Clear();
#ifdef XML_USE_STL
		string vn;
		string vv;
#else
		char* vn;
		char* vv;
#endif
		XMLElement* owner;
		bool Temporal;


	};



class XMLComment
	{
	public:

		// constructors/destructor
		XMLComment(XMLElement* p = 0,int ElementPosition = -1,const char* ht = 0);
		operator const char*() const;
		void SetComment(const char* ht);
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();
#ifdef XML_USE_STL
		int Compare(XMLComment&);
#else
		int Compare(XMLComment*);
#endif

		XMLComment(const XMLComment&);
		XMLComment& operator =(const XMLComment&);
		~XMLComment();

		XMLComment* Duplicate();
		void SetParent(XMLElement* p,int ep);
		int GetEP() const;

	private:

		XMLElement* parent;
#ifdef XML_USE_STL
		string c;
#else
		char* c;
#endif
		int ep; // Element Position (Before)
	};


class XMLContent
	{
	public:

		// constructors/destructor
		XMLContent(XMLElement* p = 0,int ElementPosition = -1,const char* ht = 0,int NoDecode = 0,int BinarySize = 0);
		operator const char*();
		size_t GetValue(char*,int NoDecode = 0) const; 
		bool GetBinaryValue(char**o,int* len);
		void SetValue(const char*,int NoDecode = 0,int BinarySize = 0);
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();
#ifdef XML_USE_STL
		int Compare(XMLContent&);
#else
		int Compare(XMLContent*);
#endif

		XMLContent(const XMLContent&);
		XMLContent& operator =(const XMLContent&);
		~XMLContent();

		XMLContent* Duplicate();
		void SetParent(XMLElement* p,int ep);
		int GetEP() const;

	private:

		XMLElement* parent;
#ifdef XML_USE_STL
		string c;
#else
		char* c;
#endif
		int ep; // Element Position (Before)
	};

class XMLCData
	{
	public:

		// constructors/destructor
		XMLCData(XMLElement* p = 0,int ElementPosition = -1,const char* ht = 0);
		operator const char*() const;
		void SetCData(const char* ht);
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();
#ifdef XML_USE_STL
		int Compare(XMLCData&);
#else
		int Compare(XMLCData*);
#endif

		XMLCData(const XMLCData&);
		XMLCData& operator =(const XMLCData&);
		~XMLCData();

		XMLCData* Duplicate();
		void SetParent(XMLElement* p,int ep);
		int GetEP() const;

	private:

		XMLElement* parent;
#ifdef XML_USE_STL
		string c;
#else
		char* c;
#endif
		int ep; // Element Position (Before)
	};

class XML
	{
	public:

		// constructors/destructor

		XML();
		XML(const char* file,XML_LOAD_MODE LoadMode = XML_LOAD_MODE_LOCAL_FILE,class XMLTransform* eclass = 0,class XMLTransformData* edata = 0);
#ifndef LINUX
		XML(const wchar_t* file,XML_LOAD_MODE LoadMode = XML_LOAD_MODE_LOCAL_FILE,class XMLTransform* eclass = 0,class XMLTransformData* edata = 0);
#endif
		void Version(XML_VERSION_INFO*);
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();
		int Compare(XML*);

		XML(XML& x);
		XML& operator =(XML&);
		~XML();

		//      static void Kill(char* tf);
#ifdef LINUX
		int PhantomLoad(const char* file);
#else
		int PhantomLoad(const char* file,bool IsUnicode = false,bool UseMap = false);
#endif
		int PhantomElement(FILE*fp,class XMLElement* r,unsigned long long StartP,unsigned long long EndP);

		static int DoMatch(const char *text, char *p, bool IsCaseSensitive = false);
		static bool VMatching(const char *text, char *p, bool IsCaseSensitive = false);
		static bool TestMatch(const char* item1,const char* comp,const char* item2);
		static Z<char>* ReadToZ(const char*,class XMLTransform* eclass = 0,class XMLTransformData* edata = 0,bool IsU = 0);
		static int Convert2HexCharsToNumber(int c1, int c2);
		static XMLElement* Paste(char* txt = 0);

		XML_PARSE_STATUS ParseStatus(int* = 0);
		void SetUnicode(bool x);
		void SaveOnClose(bool);
		int Load(const char* data,XML_LOAD_MODE LoadMode = XML_LOAD_MODE_LOCAL_FILE,class XMLTransform* eclass = 0,class XMLTransformData* edata = 0);
		size_t LoadText(const char*);
		size_t LoadText(const wchar_t*);
		static int PartialLoad(const char* file,const char* map);
		static XMLElement * PartialElement(const char* file,const char* map);
		int Save(const char* file = 0,XML_SAVE_MODE SaveMode = XML_SAVE_MODE_DEFAULT,XML_TARGET_MODE TargetMode = XML_TARGET_MODE_FILE,class XMLTransform* eclass = 0,class XMLTransformData* edata = 0); // Default, do not encode already encoded
		void Export(FILE* fp,XML_SAVE_MODE SaveMode,XML_TARGET_MODE TargetMode = XML_TARGET_MODE_FILE,XMLHeader *hdr = 0,class XMLTransform* eclass = 0,class XMLTransformData* edata = 0);
		void SetExportFormatting(XMLEXPORTFORMAT* xf);

		void Lock(bool);
		int RemoveTemporalElements();
#ifdef XML_OPTIONAL_IMPORTDB
		static XMLElement* ImportDB(IMPORTDBPARAMS* );
#endif
#ifdef XML_OPTIONAL_IMPORTRKEY
		static XMLElement* ImportRKey(IMPORTRKEYDATA*);
#endif

#ifdef XML_USE_STL
		void SetRootElement(XMLElement&);
		XMLElement RemoveRootElementAndKeep();
		XMLElement& GetRootElement();
		XMLHeader& GetHeader();
		void SetHeader(XMLHeader& h);
#else
		void SetRootElement(XMLElement*);
		XMLElement* RemoveRootElementAndKeep();
		XMLElement* GetRootElement();
		XMLHeader* GetHeader();
		void SetHeader(XMLHeader* h);
#endif

		static size_t XMLEncode(const char* src,char* trg);
		static size_t XMLDecode(const char* src,char* trg);
		size_t XMLGetValue(const char* section,const char* attr,char* put,size_t maxlen);
		void XMLSetValue(const char* section,const char* attr,char* put);

		// Query functions
		int XMLQuery(const char* rootsection,const char* expression,XMLElement** rv,unsigned int deep = 0xFFFFFFFF);

	private:

		void Init();
		void Clear();

		XML_PARSE_STATUS iParseStatus; // 0 Valid , 1 Error but recovered, 2 fatal error
		int iParseStatusPos;
#ifndef LINUX
		bool IsFileU; // unicode file
#endif

#ifdef XML_USE_STL
		string f;          // filename
		XMLHeader hdr;
		XMLElement _root;
#else
		char* f;          // filename
		XMLHeader* hdr;   // header (one)
		XMLElement* root; // root element (one)
#endif

		bool SOnClose;
		// For Windows


	};


// public functions
size_t XMLGetString(const char* section,const char* Tattr,const char* defv,char*out,const size_t maxlen,const char* xml,XML* af = 0);
int     XMLGetInt(const char* item,const char* attr,const int defv,const char* xml,XML* af = 0);
unsigned int XMLGetUInt(const char* item,const char* attr,const unsigned int defv,const char* xml,XML* af = 0);
#ifdef _WIN32
long long XMLGetInt64(const char* item,const char* attr,const long long defv,const char* xml,XML* af = 0);
unsigned long long XMLGetUInt64(const char* item,const char* attr,const unsigned long long defv,const char* xml,XML* af = 0);
#endif
float   XMLGetFloat(const char* item,const char* attr,const float defv,const char* xml,XML* af = 0);
size_t     XMLGetBinaryData(const char* item,const char* attr,const char* defv,char*out,const size_t maxlen,const char* xml,XML* af = 0);

int    XMLSetString(const char* section,const char* Tattr,char*out,const char* xml,XML* af = 0);
int    XMLSetInt(const char* section,const char* attr,int v,const char* xml,XML* af = 0);
int    XMLSetUInt(const char* section,const char* attr,unsigned int v,const char* xml,XML* af = 0);
#ifdef _WIN32
int    XMLSetString(const char* section,const char* Tattr,wchar_t*out,const char* xml,XML* af = 0);
int    XMLSetInt64(const char* section,const char* attr,long long v,const char* xml,XML* af = 0);
int    XMLSetUInt64(const char* section,const char* attr,unsigned long long v,const char* xml,XML* af = 0);
#endif
int    XMLSetFloat(const char* section,const char* attr,float v,const char* xml,XML* af = 0);
int    XMLSetBinaryData(const char* section,const char* attr,char* data,int len,const char* xml,XML* af = 0);

int XMLRenameElement(const char* section,const char* newname,const char* xml,XML* af = 0);

#ifndef __SYMBIAN32__
#ifdef XML_USE_STL
int    XMLGetAllVariables(const char* section,vector<char*>* vnames,vector<char*>* vvalues,const char*xml);
int    XMLGetAllItems(const char* section,vector<char*>* vnames,const char*xml);
#else
int    XMLGetAllVariables(const char* section,char** vnames,char** vvalues,const char*xml);
int    XMLGetAllItems(const char* section,char** vnames,const char*xml);
#endif
#endif



// XMLTransform class

class XMLTransformData
	{
	public:
		XMLTransformData() {}
	};

class XMLTransform
	{
	public:

		XMLTransform(XMLTransformData*) { }
		virtual ~XMLTransform() {}
		virtual size_t Encrypt(const char*src,size_t srclen,int srctype,char* dst,size_t dstlen,XMLTransformData* data = 0) = 0;
		virtual size_t Decrypt(const char*src,size_t srclen,int srctype,char* dst,size_t dstlen,XMLTransformData* data = 0) = 0;

	};

class XMLHelper
	{
	public:

		// static functions
		static char* FindXMLClose(char* s);
		static XMLElement* ParseElementTree(XMLHeader* hdr,XMLElement* parent,char* tree,char** EndValue,XML_PARSE_STATUS& iParseStatus);
		static void AddBlankVariable(XMLElement* parent,char *a2,int Pos);
		static int pow(int P,int z);



	};

#ifdef XML_USE_NAMESPACE
};
#endif


#endif // _XML_H


