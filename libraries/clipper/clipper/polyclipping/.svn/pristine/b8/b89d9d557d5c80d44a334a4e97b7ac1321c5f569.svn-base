
extern "C"{
	__declspec(dllimport) void * __stdcall ClipperCreate(void);
	__declspec(dllimport) bool  __stdcall ClipperFree(void *);
	__declspec(dllimport) bool __stdcall ClipperClear(void *);
	__declspec(dllimport) bool __stdcall ClipperAddPolygon(void *, float *, int);
	__declspec(dllimport) bool __stdcall ClipperExecute(void *, int , int *);
	__declspec(dllimport) bool __stdcall ClipperSolution(void *, float *, int);
}

enum {ptSubject, ptClip};
enum {ctIntersection, ctUnion, ctDifference, ctXor};
