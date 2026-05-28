/**@file  		ReflectionWrap.h
* @brief		Reflection wrapper templates
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#pragma once
#include <array>
#include <utility>
#include <cstddef>
#include <type_traits>

#include "TemplateWrap.h"
#include "CField.h"
#include "CClass.h"
#include "CReflection.h"

namespace XReflect
{
	//================================================================================
	// Parameter extraction helpers
	//================================================================================
	///< Function calling wrapper
	template<typename T> struct ArgFetcher { T& operator()( void* p ){ return *(T*)p; } };
	template<typename T> struct ArgFetcher<T   &&> { T&& operator()( void* p ){ return std::move( *(T*)p ); } };
	template<typename T> struct ArgFetcher<T	&> : public ArgFetcher<T		> {};
	template<> struct ArgFetcher<const char		&> : public ArgFetcher<char		> {};
	template<> struct ArgFetcher<const int8		&> : public ArgFetcher<int8		> {};
	template<> struct ArgFetcher<const int16	&> : public ArgFetcher<int16	> {};
	template<> struct ArgFetcher<const int32	&> : public ArgFetcher<int32	> {};
	template<> struct ArgFetcher<const int64	&> : public ArgFetcher<int64	> {};
	template<> struct ArgFetcher<const long		&> : public ArgFetcher<long		> {};
	template<> struct ArgFetcher<const wchar_t	&> : public ArgFetcher<wchar_t	> {};
	template<> struct ArgFetcher<const char16_t	&> : public ArgFetcher<char16_t	> {};
	template<> struct ArgFetcher<const char32_t	&> : public ArgFetcher<char32_t	> {};
	template<> struct ArgFetcher<const uint8	&> : public ArgFetcher<uint8	> {};
	template<> struct ArgFetcher<const uint16	&> : public ArgFetcher<uint16	> {};
	template<> struct ArgFetcher<const uint32	&> : public ArgFetcher<uint32	> {};
	template<> struct ArgFetcher<const uint64	&> : public ArgFetcher<uint64	> {};
	template<> struct ArgFetcher<const ulong	&> : public ArgFetcher<ulong	> {};
	template<> struct ArgFetcher<const float	&> : public ArgFetcher<float	> {};
	template<> struct ArgFetcher<const double	&> : public ArgFetcher<double	> {};

	///< Fetch function return&parameter types
	template<typename RetType, typename... Param>
	inline STypeInfoArray MakeFunArg()
	{
		static STypeInfo aryInfo[] =
		{ GetTypeInfo<Param>()..., GetTypeInfo<RetType>() };
		return { aryInfo, std::size( aryInfo ) };
	}

	//================================================================================
	// Construct, copy, and move helpers
	//================================================================================
	///< Copy assign
	typedef void* ( *GetVirtualTableFun )( void* );
	template<typename _T, bool bDuplicatable> struct TCopy 
	{ TCopy( void* pDest, void* pSrc ) { *(_T*)pDest = *(_T*)pSrc; } };
	template<typename _T> struct TCopy<_T, false> 
	{ TCopy( void* pDest, void* pSrc ) { throw( "Can not duplicate object" ); } };
	template<typename _T, bool bMovable> struct TMove 
	{ TMove( void* pDest, void* pSrc ) { *(_T*)pDest = std::move( *(_T*)pSrc ); } };
	template<typename _T> struct TMove<_T, false> 
	{ TMove( void* pDest, void* pSrc ) { throw( "Can not move object" ); } };

	// Construct object
	template<typename _T, bool bConstructable>
	struct TConstruct
	{
		static void New( int32 nIndex, void* pObj, void** aryArg )
		{
			_T::Construct( nIndex, (_T*)pObj, aryArg );
		}
		
		static void* New( int32 nIndex, void** aryArg )
		{
			_T* pObj = nullptr;
			_T::New( nIndex, pObj, aryArg );
			return pObj;
		}
	};

	template<typename _T>
	struct TConstruct<_T, false>
	{
		static void New( int32 nIndex, void* pObj, void** aryArg ) { throw( "Can not construct object" ); }
		static void* New( int32 nIndex, void** aryArg ) { throw( "Can not construct object" ); }
	};

	template<typename... ParamTypes> class TConstructionTypes;
	template<> class TConstructionTypes<> {};
	template<typename Cur, typename... Remains>
	class TConstructionTypes<Cur, Remains...> : public TConstructionTypes<Remains...>
	{ public: typedef TConstructionTypes<Remains...> RemainTypes; typedef Cur CurType; };

	///< New
	template<typename _T, bool bEnable> struct TNew
	{
		static void New( void* pDest )
		{
			SFunctionTable* pTable = nullptr;
			new( pDest ) _T( TConstructionTypes<>(), pTable, false, nullptr );
			if( !pTable ) return;
			( (SVirtualClassInstance*)pDest )->pVirtualTable = pTable;
		}

		static _T* New()
		{
			SFunctionTable* pTable = nullptr;
			_T* pObj = new _T( TConstructionTypes<>(), pTable, false, nullptr );
			if( pTable )
				( (SVirtualClassInstance*)pObj )->pVirtualTable = pTable;
			return pObj;
		}
	};

	template<typename _T> struct TNew<_T, false>
	{
		static void New( void* pDest ) { throw( "Can not duplicate object" ); }
		static _T* New() { throw( "Can not duplicate object" ); }
	};

	///< TCopyNew
	template<typename _T, bool bEnable> struct TCopyNew
	{
		static void New( void* pDest, void* pSrc )
		{
			SFunctionTable* pTable = nullptr;
			new( pDest ) _T( TConstructionTypes<const _T&>(), pTable, false, &pSrc );
			if( !pTable ) return;
			( (SVirtualClassInstance*)pDest )->pVirtualTable = pTable;
		}

		static _T* New( void* pSrc )
		{
			SFunctionTable* pTable = nullptr;
			_T* pObj = new _T( TConstructionTypes<const _T&>(), pTable, false, &pSrc );
			if( pTable )
				( (SVirtualClassInstance*)pObj )->pVirtualTable = pTable;
			return pObj;
		}
	};

	template<typename _T> struct TCopyNew<_T, false>
	{
		static void New( void* pDest, void* pSrc ) { throw( "Can not duplicate object" ); }
		static _T* New( void* pSrc ) { throw( "Can not duplicate object" ); }
	};

	///< TMoveNew
	template<typename _T, bool bEnable> struct TMoveNew
	{
		static void New( void* pDest, void* pSrc )
		{
			SFunctionTable* pTable = nullptr;
			new( pDest ) _T( TConstructionTypes<_T&&>(), pTable, false, &pSrc );
			if( !pTable ) return;
			( (SVirtualClassInstance*)pDest )->pVirtualTable = pTable;
		}

		static _T* New( void* pSrc )
		{
			SFunctionTable* pTable = nullptr;
			_T* pObj = new _T( TConstructionTypes<_T&&>(), pTable, false, &pSrc );
			if( pTable )
				( (SVirtualClassInstance*)pObj )->pVirtualTable = pTable;
			return pObj;
		}
	};

	template<typename _T> struct TMoveNew<_T, false>
	{
		static void New( void* pDest, void* pSrc ) { throw( "Can not duplicate object" ); }
		static _T* New( void* pSrc ) { throw( "Can not duplicate object" ); }
	};

	//================================================================================
	// Wraps
	//================================================================================
	template<typename ClassType, bool bConstructable>
	struct TConstructWrap : public ClassType
	{
		// Use universal references and perfect forwarding to pass arguments while preserving types
		template<typename ConstructionTypes, typename... FetchedTypes>
		TConstructWrap( ConstructionTypes, 
			SFunctionTable** ppVTable, void** aryArg, FetchedTypes&& ... p )
			: TConstructWrap( typename ConstructionTypes::RemainTypes(), 
				ppVTable, aryArg + 1, std::forward<FetchedTypes>( p )...,
				ArgFetcher<typename ConstructionTypes::CurType>()( aryArg[0] ) ) {
		}

		template<typename... FetchedTypes>
		TConstructWrap( TConstructionTypes<>, 
			SFunctionTable** ppVTable, void**, FetchedTypes&&...p )
			: ClassType( std::forward<FetchedTypes>( p )... )
		{
			if( !ppVTable )
				return;
			*ppVTable = ( (SVirtualClassInstance*)this )->pVirtualTable;
		}
	};

	template<typename ClassType>
	struct TConstructWrap<ClassType, false>
	{
	};

	///< Construct the object
	template<typename ClassType>
	struct TObjectConstruct : public IClassDefinition
	{
	public:
		virtual void Assign( void* pDest, void* pSrc ) override
		{
			TCopy<ClassType, std::is_copy_assignable_v<ClassType>>( pDest, pSrc );
		}

		virtual void Move( void* pDest, void* pSrc ) override
		{
			TMove<ClassType, std::is_move_assignable_v<ClassType>>( pDest, pSrc );
		}

		virtual void Construct( int32 nIndex, void* pObj, void** aryArg ) override
		{
			TConstruct<ClassType, ClassType::bConstructable>::New( nIndex, pObj, aryArg );
		}

		virtual void Destruct( void* pObj ) override
		{
			static_cast<ClassType*>( pObj )->~ClassType();
		}

		virtual void* New( int32 nIndex, void** aryArg ) override
		{
			return TConstruct<ClassType, ClassType::bConstructable>::New( nIndex, aryArg );;
		}

		virtual void Delete( void* pObj ) override
		{
			delete (ClassType*)pObj;
		}
	};

	//================================================================================
	// Helper classes and functions for inheritance and derivation
	//================================================================================
	template<typename Derive, typename...Base> struct TBaseClassOffset {};
	template<typename Derive> struct TBaseClassOffset<Derive> 
	{
		static void GetOffsets( ptrdiff_t* ary ) {}
		static void GetBaseTypes( const char** ary ) {}
	};

	template<typename Derive, typename First, typename...Base>
	struct TBaseClassOffset<Derive, First, Base...>
	{
		static void GetOffsets( ptrdiff_t* ary )
		{
			First* pBaseAddress = static_cast<First*>((Derive*)0x40000000);
			*ary = ( (ptrdiff_t)pBaseAddress) - 0x40000000;
			TBaseClassOffset<Derive, Base...>::GetOffsets( ++ary );
		}

		static void GetBaseTypes( const char** ary )
		{
			*ary = TypeName<First>();
			TBaseClassOffset<Derive, Base...>::GetBaseTypes( ++ary );
		}
	};

	template<typename _Derive, typename ... _Base>
	class TInheritInfo
	{
	public:
		enum { size = sizeof...( _Base ) };

		static std::array<ptrdiff_t, size + 1> Offsets()
		{
			std::array<ptrdiff_t, size + 1> result;
			TBaseClassOffset<_Derive, _Base...>::GetOffsets( &result[0] );
			return result;
		}

		static std::array<const char*, size + 1> BaseTypes()
		{
			std::array<const char*, size + 1> result;
			TBaseClassOffset<_Derive, _Base...>::GetBaseTypes( &result[0] );
			return result;
		}
	};

	//================================================================================
	// Helper classes and functions for registering global/static/class member functions
	//================================================================================
	template< typename Type > struct TFetchResult
	{
		template<typename FunctionType, typename... Param>
		struct TCall
		{
			static void Call( FunctionType funCall, void* pRetBuf, Param...p )
			{ 
				if( pRetBuf )
					new( pRetBuf ) Type( funCall( p... ) );
				else
					funCall( p... );
			}
		};
	};

	template< typename Type > struct TFetchResult<Type&>
	{
		template<typename FunctionType, typename... Param>
		struct TCall
		{
			static void Call( FunctionType funCall, void* pRetBuf, Param...p )
			{ 
				if( pRetBuf )
					*(Type**)pRetBuf = &( funCall( p... ) );
				else
					funCall( p... );
			}
		};
	};

	template<> struct TFetchResult<void>
	{
		template<typename FunctionType, typename... Param>
		struct TCall
		{
			static void Call( FunctionType funCall, void* pRetBuf, Param...p )
			{ funCall( p... ); }
		};
	};

	template<typename RetType, typename... RemainParam> 
	struct TFunctionCaller {};

	template<typename RetType> 
	struct TFunctionCaller<RetType>
	{
		template<typename FunctionType, typename... FetchParam>
		struct TCaller
		{
			static void CallFun( size_t nIndex, FunctionType funCall, 
				void* pRetBuf, void** pArgArray, FetchParam...p )
			{
				typedef TFetchResult<RetType> FetchType;
				typedef typename FetchType::template TCall<FunctionType, FetchParam...> CallType;
				CallType::Call( funCall, pRetBuf, p... );
			}
		};
	};

	template<typename RetType, typename FirstParam, typename... RemainParam>
	struct TFunctionCaller<RetType, FirstParam, RemainParam...>
	{
		template<typename FunctionType, typename... FetchParam>
		struct TCaller
		{
			static void CallFun( size_t nIndex, FunctionType funCall,
				void* pRetBuf, void** pArgArray, FetchParam...p )
			{
				FirstParam f = ArgFetcher<FirstParam>()( pArgArray[nIndex] );
				typedef TFunctionCaller<RetType, RemainParam...> NextFunCaller;
				typedef typename NextFunCaller::template TCaller<FunctionType, FetchParam..., FirstParam> Caller;
				Caller::CallFun( nIndex + 1, funCall, pRetBuf, pArgArray, p..., f );
			}
		};
	};

	template< typename FunctionType, typename RetType, typename... Param >
	inline void CreateGlobalFunWrap( RetType (*)( Param... ), FunctionType,
		const char* szNameSpace, const char* szType, const char* szName, const CFieldTags& Tag )
	{
		TRegisterTypes<RetType, Param...>();
		typedef TFunctionCaller<RetType, Param...> FunctionCaller;
		typedef typename FunctionCaller::template TCaller<FunctionType> Caller;
		CallWrapFun funWrap = []( void* pRetBuf, void** pArgArray )
			{ Caller::CallFun( 0, FunctionType(), pRetBuf, pArgArray); };
		STypeInfoArray InfoArray = MakeFunArg<RetType, Param...>();
		CReflection::GetInstance().RegisterGlobalFunction( 
			Tag, szNameSpace, funWrap, InfoArray, szType, szName );
	}
	
	template< typename FunctionType, typename RetType, typename ClassType, typename... Param >
	inline void CreateClassFunWrap( 
		RetType (*)( ClassType*, Param... ), FunctionType, const char* szName, const CFieldTags& Tag )
	{
		TRegisterTypes<RetType, Param...>();
		typedef TFunctionCaller<RetType, ClassType*, Param...> FunctionCaller;
		typedef typename FunctionCaller::template TCaller<FunctionType> Caller;
		CallWrapFun funWrap = []( void* pRetBuf, void** pArgArray )
			{ Caller::CallFun( 0, FunctionType(), pRetBuf, pArgArray); };
		STypeInfoArray InfoArray = MakeFunArg<RetType, ClassType*, Param...>();
		CReflection::GetInstance().RegisterClassFunction( Tag, funWrap, InfoArray, szName );
	}

	template< typename Type > 
	struct TOrgFunResult
	{
		template<typename CallBackWrap, typename FunctionType, typename... Param>
		struct TCall
		{
			static void Call( FunctionType funCall, void* pRetBuf, CallBackWrap* pObj, Param...p )
			{
				Type result = ( pObj->*funCall )( p... );
				new( pRetBuf ) Type( result );
			}
		};
	};

	template< typename Type > 
	struct TOrgFunResult<Type&>
	{
		template<typename CallBackWrap, typename FunctionType, typename... Param>
		struct TCall
		{
			static void Call( FunctionType funCall, void* pRetBuf, CallBackWrap* pObj, Param...p )
			{
				*(Type**)pRetBuf = &( ( pObj->*funCall )( p... ) );
			}
		};
	};

	template<> 
	struct TOrgFunResult<void>
	{
		template<typename CallBackWrap, typename FunctionType, typename... Param>
		struct TCall
		{
			static void Call( FunctionType funCall, void* pRetBuf, CallBackWrap* pObj, Param...p )
			{
				( pObj->*funCall )( p... );
			}
		};
	};

	template<typename RetType, typename... RemainParam>
	struct TOrgFunParam {};

	template<typename RetType>
	struct TOrgFunParam<RetType>
	{
		template<typename CallBackWrap, typename FunctionType, typename... FetchParam>
		struct TCaller
		{
			static void CallFun( size_t nIndex, FunctionType funCall,
				void* pRetBuf, CallBackWrap* pObj, void** pArgArray, FetchParam...p )
			{
				typedef TOrgFunResult<RetType> OrgFunCaller;
				typedef typename OrgFunCaller::template TCall<CallBackWrap, FunctionType, FetchParam...> Caller;
				Caller::Call( funCall, pRetBuf, pObj, p... );
			}
		};
	};

	template<typename RetType, typename FirstParam, typename... RemainParam>
	struct TOrgFunParam<RetType, FirstParam, RemainParam...>
	{
		template<typename CallBackWrap, typename FunctionType, typename... FetchParam>
		struct TCaller
		{
			static void CallFun( size_t nIndex, FunctionType funCall,
				void* pRetBuf, CallBackWrap* pObj, void** pArgArray, FetchParam...p )
			{
				FirstParam f = ArgFetcher<FirstParam>()( pArgArray[nIndex] );
				typedef TOrgFunParam<RetType, RemainParam...> NextFunCaller;
				typedef typename NextFunCaller::template TCaller<CallBackWrap, FunctionType, FetchParam..., FirstParam> Caller;
				Caller::CallFun( nIndex + 1, funCall, pRetBuf, pObj, pArgArray, p..., f );
			}
		};
	};

	//================================================================================
	// Helper classes for virtual function (callback) registration
	//================================================================================
	template< typename T >
	struct TCallBack
	{
		static T OnCall( const CField* pField, void** pArgArray )
		{
			T ReturnValue;
			pField->CallBack( &ReturnValue, pArgArray );
			return ReturnValue;
		}
	};

	template<typename T>
	struct TCallBack<T&>
	{
		static T& OnCall( const CField* pField, void** pArgArray )
		{
			T* pReturnValue;
			pField->CallBack( &pReturnValue, pArgArray );
			return *pReturnValue;
		}
	};

	template<>
	struct TCallBack<void>
	{
		static void OnCall( const CField* pField, void** pArgArray )
		{
			pField->CallBack( nullptr, pArgArray );
		}
	};

	template<typename OverrideClassType>
	class TCallBackBinder
	{
		template<typename RetType, typename ClassType, typename... Param >
		class TCallBackWrap
		{
		public:
			static const CField*& GetCallBackField()
			{
				static const CField* s_nCallBackField = nullptr;
				return s_nCallBackField;
			}

			RetType BootFunction( Param ... p )
			{
				TCallBackWrap* pThis = this;
				void* pCallArray[] = { &pThis, &p..., nullptr };
				return TCallBack<RetType>::OnCall( GetCallBackField(), pCallArray );
			}

			typedef decltype( &TCallBackWrap::BootFunction ) FunctionType;

		public:
			static void Call( void* pRetBuf, void** pArgArray, FunctionType funOrg )
			{
				TCallBackWrap* pObj = *(TCallBackWrap**)pArgArray[0];
				typedef TOrgFunParam<RetType, Param...> OrgFunctionCaller;
				typedef typename OrgFunctionCaller::template TCaller<TCallBackWrap, FunctionType> Caller;
				Caller::CallFun( 0, funOrg, pRetBuf, pObj, pArgArray + 1 );
			}
		};

	public:
		template<typename RetType, typename ClassType, typename... Param >
		static void Bind( bool bPureVirtual, RetType(ClassType::*pFun)(Param...), 
			const char* szFunName, const CFieldTags& Tag )
		{
			TRegisterTypes<RetType, Param...>();
			typedef void ( ClassType::*FunctionType )();
			FunctionType funCall = (FunctionType)pFun;
			VirtualFunCallback funCallback = []( void* pObj, void* pContext )
			{ ( ((ClassType*)pObj)->*(*(FunctionType*)pContext) )(); };
			typedef TCallBackWrap<RetType, ClassType, Param...> CallBackWrap;
			STypeInfoArray InfoArray = MakeFunArg<RetType, ClassType*, Param...>();
			auto funBoot = (FunctionPointer)FunctionTypeToIntValue( &CallBackWrap::BootFunction );
			uint32 nFunIndex = FindVirtualFunction( sizeof(ClassType), funCallback, &funCall );
			CallbackWrapFun funWrap = (CallbackWrapFun)&CallBackWrap::Call;
			CallBackWrap::GetCallBackField() = CReflection::GetInstance().RegisterClassCallback(
				Tag, funWrap, funBoot, nFunIndex, bPureVirtual, InfoArray, szFunName );
		}

		template< typename ClassType, typename RetType, typename... Param >
		static inline void Bind(bool bPureVirtual, RetType (ClassType::*pFun)(Param...) const,
			const char* szFunName, const CFieldTags& Tag )
		{
			typedef RetType(ClassType::*FunctionType)(Param...);
			Bind( bPureVirtual, (FunctionType)pFun, szFunName, Tag );
		}
	};

	//================================================================================
	// Helper classes for Destructor registration
	//================================================================================
	template< typename ClassType >
	class TDestructorWrap
	{
		static const CField*& GetCallBackField()
		{
			static const CField* s_nCallBackField = nullptr;
			return s_nCallBackField;
		}

		void Wrap( uint32 p0 )
		{
			void* pArg[] = { this, &p0 };
			GetCallBackField()->CallBack( nullptr, pArg );
		}

	public:
		static void Bind( const CFieldTags& Tag )
		{
			struct Derive : public ClassType { virtual ~Derive() {} };
			if( sizeof( Derive ) != sizeof(ClassType) )
				throw "the class have not virtual table";
			VirtualFunCallback funCallback = 
				[]( void* pObj, void* ){ ((Derive*)pObj)->~Derive(); };
			uint32 nIndex = FindVirtualFunction( sizeof( Derive ), funCallback, nullptr );
			auto funBoot = (FunctionPointer)FunctionTypeToIntValue( &TDestructorWrap::Wrap );
			STypeInfo aryInfo[2] = { GetTypeInfo<ClassType*>(), GetTypeInfo<void>() };
			STypeInfoArray TypeInfo = { aryInfo, sizeof( aryInfo )/sizeof( STypeInfo ) };
			DtorWrapFun funWrap = []( void* pObject ) { ( (Derive*)pObject )->~Derive(); };
			GetCallBackField() = CReflection::GetInstance().
				RegisterDestructor( Tag, funWrap, funBoot, nIndex, TypeInfo );
		}
	};

	//================================================================================
	// Helper classes and functions for class member get/set
	//================================================================================
	template<typename C, typename F>
	struct ToMemberFuntion { typedef F T; };

	template<typename C, typename R, typename ... P>
	struct ToMemberFuntion<C, R( P... )> { typedef R( C::* T )(P...); };

	template<typename C, typename R, typename ... P>
	struct ToMemberFuntion<C, R( P... )const> { typedef R( C::* T )(P...) const; };
	
	// Helper trait: convert C-style array to nested std::array
	template<typename T>
	struct TArrayToStdArray { using type = T; };
	
	template<typename T, size_t N>
	struct TArrayToStdArray<T[N]> {
	    using type = std::array<typename TArrayToStdArray<T>::type, N>;
	};
	
	template<bool bAddressable>
	struct TMemberTypeFetcher
	{
	    template<typename MemberType>
	    static MemberType GetMemberType( MemberType );
	};
	
	template<>
	struct TMemberTypeFetcher<true>
	{
	    template<typename MemberType>
	    static MemberType GetMemberType( const MemberType& );
	
	    // 1D array (recursion terminator): ElemType is not an array type
	    template<typename ElemType, size_t n>
	    static typename std::enable_if<!std::is_array<ElemType>::value, std::array<ElemType, n>>::type
	    GetMemberType( ElemType( &ary )[n] );
	
	    // Multi-dimensional array (recursive expansion): SubArray is an array type (dimension reduction)
	    template<typename SubArray, size_t n>
	    static typename std::enable_if<std::is_array<SubArray>::value,
	        std::array<typename TArrayToStdArray<SubArray>::type, n>>::type
	    GetMemberType( SubArray( &ary )[n] );
	};

	template< typename MemberType >
	class TMemberReturnType
	{
		typedef typename std::remove_reference<MemberType>::type OrgType;
		template<bool bReference> struct TReturnType;
		template<> struct TReturnType<false> { typedef OrgType Type; };
		template<> struct TReturnType<true>  { typedef const OrgType& Type; };
	public:
		constexpr static bool bNeedClassReference =
			TTypeInfo<MemberType>::eType == eDT_class &&
			TTypeInfo<MemberType>::nConstMaskCount == 1;
		typedef typename TReturnType<bNeedClassReference>::Type ReturnType;
	};

	///< Data member access(read&write) wrapper
	template< typename MemberAccess, bool bAddressable >
	class TMemberWrap
	{
	public:
		typedef typename MemberAccess::MemberType MemberType;

		static void Get( void* pObj, void* pRetBuf )
		{
			auto pObject = (typename MemberAccess::ClassType*)pObj;
			// Fetch the member value
			MemberType v = MemberAccess::Access( pObject, nullptr );
			// Prefer copy construction; fall back to move if copy is not available
			if constexpr( std::is_copy_constructible_v<MemberType> )
			{
				new( pRetBuf ) MemberType( v );
			}
			else if constexpr( std::is_move_constructible_v<MemberType> )
			{
				new( pRetBuf ) MemberType( std::move( v ) );
			}
			else
			{
				static_assert( std::is_copy_constructible_v<MemberType> || std::is_move_constructible_v<MemberType>,
					"MemberType must be copyable or movable for reflection Get operation" );
			}
		}

		static void Set( void* pObj, void* pData )
		{
			auto pObject = ( typename MemberAccess::ClassType* )pObj;
			MemberAccess::Access( pObject, &ArgFetcher<MemberType>()( pData ) );
		}
	};

	template< typename MemberAccess >
	class TMemberWrap<MemberAccess, true>
	{
	public:
		typedef std::remove_reference_t<typename MemberAccess::MemberType> MemberType;

		static void Get( void* pObj, void* pRetBuf )
		{
			auto pObject = ( typename MemberAccess::ClassType* )pObj;
			*(const MemberType**)pRetBuf = &MemberAccess::Access( pObject, nullptr );
		}

		static void Set( void* pObj, void* pData )
		{
			auto pObject = ( typename MemberAccess::ClassType* )pObj;
			MemberAccess::Access( pObject, &ArgFetcher<MemberType>()( pData ) );
		}
	};
	
	template<typename MemberAccess>
	inline void CreateMemberWrap( const char* szName, const CFieldTags& Tag )
	{
		typedef typename MemberAccess::MemberType MemberType;
		TRegisterTypes<MemberType>();
		static STypeInfo Param = GetTypeInfo<MemberType>();
		static STypeInfo aryInfo[] = { GetTypeInfo<typename MemberAccess::ClassType*>(), Param };
		constexpr static EMemberQualifier Qualifier = MemberAccess::s_eQualifier;
		constexpr static bool bAddressable = MemberAccess::s_bAddressable;
		constexpr static bool bDisableWrite = ( Qualifier & eMBQ_DisableWrite ) != 0;
		auto funGet = &TMemberWrap<MemberAccess, bAddressable>::Get;
		auto funSet = &TMemberWrap<MemberAccess, bAddressable>::Set;
		STypeInfoArray TypeInfo = { aryInfo, sizeof( aryInfo )/sizeof( STypeInfo ) };
		CReflection::GetInstance().RegisterClassMember( Tag, Qualifier, 
			funGet, bDisableWrite ? nullptr : funSet, TypeInfo, szName );
	}

	//================================================================================
	// Static member access wrapper
	//================================================================================
	template< typename MemberAccess, bool bAddressable >
	class TStaticMemberWrap
	{
	public:
		typedef typename MemberAccess::MemberType MemberType;

		static void Get( void* pObj, void* pRetBuf )
		{
			MemberType v = MemberAccess::Access( nullptr );
			if constexpr( std::is_copy_constructible_v<MemberType> )
			{
				new( pRetBuf ) MemberType( v );
			}
			else if constexpr( std::is_move_constructible_v<MemberType> )
			{
				new( pRetBuf ) MemberType( std::move( v ) );
			}
			else
			{
				static_assert( std::is_copy_constructible_v<MemberType> || std::is_move_constructible_v<MemberType>,
					"MemberType must be copyable or movable for reflection Get operation" );
			}
		}

		static void Set( void* pObj, void* pData )
		{
			MemberAccess::Access( &ArgFetcher<MemberType>()( pData ) );
		}
	};

	template< typename MemberAccess >
	class TStaticMemberWrap<MemberAccess, true>
	{
	public:
		typedef std::remove_reference_t<typename MemberAccess::MemberType> MemberType;

		static void Get( void* pObj, void* pRetBuf )
		{
			*(const MemberType**)pRetBuf = &MemberAccess::Access( nullptr );
		}

		static void Set( void* pObj, void* pData )
		{
			MemberAccess::Access( &ArgFetcher<MemberType>()( pData ) );
		}
	};

	template<typename MemberAccess>
	inline void CreateStaticMemberWrap( const char* szName, const CFieldTags& Tag, EFieldType eFieldType )
	{
		typedef typename MemberAccess::MemberType MemberType;
		TRegisterTypes<MemberType>();
		static STypeInfo Param = GetTypeInfo<MemberType>();
		static STypeInfo aryInfo[] = { GetTypeInfo<typename MemberAccess::ClassType*>(), Param };
		constexpr static EMemberQualifier Qualifier = MemberAccess::s_eQualifier;
		constexpr static bool bAddressable = MemberAccess::s_bAddressable;
		constexpr static bool bDisableWrite = Qualifier & eMBQ_DisableWrite;
		auto funGet = &TStaticMemberWrap<MemberAccess, bAddressable>::Get;
		auto funSet = &TStaticMemberWrap<MemberAccess, bAddressable>::Set;
		STypeInfoArray TypeInfo = { aryInfo, sizeof( aryInfo )/sizeof( STypeInfo ) };
		CReflection::GetInstance().RegisterStaticMember( Tag, Qualifier,
			funGet, bDisableWrite ? nullptr : funSet, TypeInfo, szName, eFieldType );
	}

	//================================================================================
	// Helper classes and functions for smart pointer registration
	//================================================================================
	// Strong smart pointer owns the object, so returning the object pointer is sufficient
	template<typename StrongPtrType>
	inline void CreateStrongPointerWrap( const CFieldTags& Tag )
	{
		typedef std::remove_reference_t<decltype (*(*(StrongPtrType*)nullptr))> ValueType;
		TRegisterTypes<ValueType>();
		static STypeInfo aryInfo[] = { GetTypeInfo<StrongPtrType*>(), GetTypeInfo<ValueType*>() };
		static GetWrapFun funGet = []( void* pObject, void* pResult ) 
			{ *(ValueType**)pResult = &*(*(StrongPtrType*)pObject); };
		STypeInfoArray TypeInfo = { aryInfo, sizeof( aryInfo )/sizeof( STypeInfo ) };
		CReflection::GetInstance().RegisterSmartPtr( Tag, funGet, TypeInfo );
	}

	// Construct strong smart pointer from weak smart pointer (default)
	template<typename WeakPtrType, typename StrongPtrType>
	inline void ConstructStrongPtrFromWeakPtr( 
		StrongPtrType* pStrongPtr, WeakPtrType& WeakPtr )
	{
		new( pStrongPtr ) StrongPtrType( WeakPtr );
	}

	// Construct strong smart pointer from weak smart pointer (std::shared_ptr specialization)
	template<typename ValueType>
	inline void ConstructStrongPtrFromWeakPtr( 
		std::shared_ptr<ValueType>* pStrongPtr, std::weak_ptr<ValueType>& WeakPtr )
	{
		new( pStrongPtr ) std::shared_ptr<ValueType>( WeakPtr.lock() );
	}

	// Weak smart pointer does not own the object, so construct a strong smart pointer first to keep the object alive
	template<typename WeakPtrType, typename StrongPtrType>
	inline void CreateWeakPointerWrap( const CFieldTags& Tag )
	{
		TRegisterTypes<StrongPtrType>();
		static STypeInfo aryInfo[] = { GetTypeInfo<WeakPtrType*>(), GetTypeInfo<StrongPtrType>() };
		static GetWrapFun funGet = []( void* pObject, void* pResult ) 
		{ 
			ConstructStrongPtrFromWeakPtr( (StrongPtrType*)pResult, *(WeakPtrType*)pObject );
		};
		STypeInfoArray TypeInfo = { aryInfo, sizeof( aryInfo )/sizeof( STypeInfo ) };
		CReflection::GetInstance().RegisterSmartPtr( Tag, funGet, TypeInfo );
	}

	//================================================================================
	// Helper functions for class iterator registration
	//================================================================================
	template<typename It>
	void CreateIteratorWrap( IClassDefinition* pDef, const CFieldTags& Tag )
	{
		auto& Reflection = CReflection::GetInstance();
		Reflection.RegisterClass( TypeName<It>(), pDef );
		using ValueType = decltype( *It() );
		TRegisterTypes<ValueType>();

		typedef It& ( *IncFun )( It* );
		struct IncCaller { It& operator()( It* pIt ) { return ++( *pIt ); } };
		CreateClassFunWrap( IncFun(), IncCaller(), ITERATOR_INC, Tag );

		typedef It& ( *DecFun )( It* );
		struct DecCaller { It& operator()( It* pIt ) { return --( *pIt ); } };
		CreateClassFunWrap( DecFun(), DecCaller(), ITERATOR_DEC, Tag );

		typedef It& ( *AddFun )( It*, uint32 );
		struct AddCaller { It& operator()( It* pIt, uint32 n )
			{ while( n ) { ++( *pIt ); n--; } return *pIt; } };
		CreateClassFunWrap( AddFun(), AddCaller(), ITERATOR_ADD, Tag );

		typedef It& ( *SubFun )( It*, uint32 );
		struct SubCaller { It& operator()( It* pIt, uint32 n )
			{ while( n ) { --( *pIt ); n--; } return *pIt; } };
		CreateClassFunWrap( SubFun(), SubCaller(), ITERATOR_SUB, Tag );

		typedef bool ( *EqualFun )( It*, const It& );
		struct EqlCaller { bool operator()( It* pIt, const It& r ) { return *pIt == r; } };
		CreateClassFunWrap( EqualFun(), EqlCaller(), ITERATOR_EQUAL, Tag );

		typedef bool ( *NotEqualFun )( It*, const It& );
		struct NeqCaller { bool operator()( It* pIt, const It& r ) { return *pIt != r; } };
		CreateClassFunWrap( NotEqualFun(), NeqCaller(), ITERATOR_NOTEQUAL, Tag );

		typedef It& ( *AssignFun )( It*, const It& );
		struct AssignCaller { It& operator()( It* pIt, const It& r ) { return *pIt = r; } };
		CreateClassFunWrap( AssignFun(), AssignCaller(), ITERATOR_ASSIGN, Tag );

		typedef ValueType& ( *ValueFun )( It* );
		struct ValueCaller { ValueType& operator()( It* pIt ) { return *( *pIt ); } };
		CreateClassFunWrap( ValueFun(), ValueCaller(), ITERATOR_VALUE, Tag );
	}
}
