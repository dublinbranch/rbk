#ifndef _TMP_QTCREATOR_SRYUTN_CLANGTOOLS_VFSO_HITHHI_MIXIN_H_AUTO
#define _TMP_QTCREATOR_SRYUTN_CLANGTOOLS_VFSO_HITHHI_MIXIN_H_AUTO

class CopyAssignable {
      public:
	CopyAssignable()  = default;
	~CopyAssignable() = default;

	CopyAssignable(const CopyAssignable&) = delete;

      protected:
	//Needed to exist so is easy to reset with *this = X();
	//CopyAssignable& operator=(const CopyAssignable&) = delete;
};

class NoCopy {
      protected:
	NoCopy()  = default;
	~NoCopy() = default;

	NoCopy(const NoCopy&) = delete;
	NoCopy& operator=(const NoCopy&) = delete;
};

#endif
