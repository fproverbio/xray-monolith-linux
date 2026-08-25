///////////////////////////////////////////////////////////////
// ini_id_loader.h
// темплейтовый класс, который загружает из ini файла 
// строку с текстовыми id, потом присваивает каждому текстовому
// id уникальный index
///////////////////////////////////////////////////////////////

#pragma once

//T_ID, T_INDEX -	тип индекса и id

//ITEM_DATA		-	структура с полями id и index типа T_ID и T_INDEX,
//					обязательно имеет конструктор с параметрами (T_INDEX index, T_ID id, LPCSTR r1, ..., LPCSTR rN)
//					N = ITEM_REC_NUM - число доп. параметров в ITEM_DATA 

//T_INIT		-	класс где определена статическая InitIdToIndex
//					функция инициализации section_name и line_name

#define TEMPLATE_SPECIALIZATION		template<u32 ITEM_REC_NUM, typename ITEM_DATA, typename T_ID, typename T_INDEX, typename T_INIT>
#define CSINI_IdToIndex CIni_IdToIndex	<ITEM_REC_NUM, ITEM_DATA, T_ID, T_INDEX, T_INIT>

TEMPLATE_SPECIALIZATION
class CIni_IdToIndex
{
public:
	typedef T_INDEX index_type;
	typedef T_ID id_type;

protected:
	typedef xr_vector<ITEM_DATA> T_VECTOR;
	static T_VECTOR* m_pItemDataVector;

	// MSVC accepts a nested explicit specialization (`template <> ... LoadItemData<0>`/
	// `<1>`) of a member template inside a class template (CIni_IdToIndex<...>) that's
	// itself still dependent on the enclosing class's own template parameters - illegal
	// in standard C++ (explicit specialization of a member template requires the
	// enclosing class template to be non-dependent first). GCC's -Wtemplate-body flags
	// it and hard-errors at first real instantiation (same bug class documented
	// repeatedly this session, e.g. object_loader.h/object_saver.h, object_comparer.h,
	// static_cast_checked.hpp). Rewritten with if constexpr - identical compile-time
	// branch, standard-legal.
	template <u32 NUM>
	static void LoadItemData(u32 count, LPCSTR cfgRecord)
	{
		if constexpr (NUM == 0)
		{
			for (u32 k = 0; k < count; k += 1)
			{
				string64 buf;
				LPCSTR id_str = _GetItem(cfgRecord, k, buf);
				char* id_str_lwr = xr_strdup(id_str);
				xr_strlwr(id_str_lwr);
				ITEM_DATA item_data(T_INDEX(m_pItemDataVector->size()), T_ID(id_str));
				m_pItemDataVector->push_back(item_data);
				xr_free(id_str_lwr);
			}
		}
		else if constexpr (NUM == 1)
		{
			for (u32 k = 0; k < count; k += 2)
			{
				string64 buf, buf1;
				LPCSTR id_str = _GetItem(cfgRecord, k, buf);
				char* id_str_lwr = xr_strdup(id_str);
				xr_strlwr(id_str_lwr);
				LPCSTR rec1 = _GetItem(cfgRecord, k + 1, buf1);
				ITEM_DATA item_data(T_INDEX(m_pItemDataVector->size()), T_ID(id_str), rec1);
				m_pItemDataVector->push_back(item_data);
				xr_free(id_str_lwr);
			}
		}
		else
		{
			// Dependent on NUM (always false for any NUM reaching here,
			// but not resolvable until instantiation) rather than a plain
			// `false` - a hard-coded `false` inside an if-constexpr branch
			// still gets checked at template-parse time regardless of
			// which branch is ever actually taken, breaking the NUM==0/
			// NUM==1 branches above too (same trap fixed the same way in
			// ini_table_loader.h's convert(), this same batch).
			STATIC_CHECK(NUM == u32(-1), Specialization_for_LoadItemData_in_CIni_IdToIndex_not_found);
			NODEFAULT;
		}
	}

	//имя секции и линии откуда будут загружаться id
	static LPCSTR section_name;
	static LPCSTR line_name;

public:
	CIni_IdToIndex();
	virtual ~CIni_IdToIndex();

	static void InitInternal();
	static const ITEM_DATA* GetById(const T_ID& str_id, bool no_assert = false);
	static const ITEM_DATA* GetByIndex(T_INDEX index, bool no_assert = false);

	static const T_INDEX IdToIndex(const T_ID& str_id, T_INDEX default_index = T_INDEX(-1), bool no_assert = false)
	{
		const ITEM_DATA* item = GetById(str_id, no_assert);
		return item ? item->index : default_index;
	}

	static const T_ID IndexToId(T_INDEX index, T_ID default_id = NULL, bool no_assert = false)
	{
		const ITEM_DATA* item = GetByIndex(index, no_assert);
		return item ? item->id : default_id;
	}

	static const T_INDEX GetMaxIndex() { return m_pItemDataVector->size() - 1; }

	//удаление статичекого массива
	static void DeleteIdToIndexData();
};


TEMPLATE_SPECIALIZATION
typename CSINI_IdToIndex::T_VECTOR* CSINI_IdToIndex::m_pItemDataVector = NULL;

TEMPLATE_SPECIALIZATION
LPCSTR CSINI_IdToIndex::section_name = NULL;
TEMPLATE_SPECIALIZATION
LPCSTR CSINI_IdToIndex::line_name = NULL;


TEMPLATE_SPECIALIZATION
CSINI_IdToIndex::CIni_IdToIndex()
{
}


TEMPLATE_SPECIALIZATION
CSINI_IdToIndex::~CIni_IdToIndex()
{
}


TEMPLATE_SPECIALIZATION
const ITEM_DATA* CSINI_IdToIndex::GetById(const T_ID& str_id, bool no_assert)
{
	typename T_VECTOR::iterator it = m_pItemDataVector->begin();
	for (; m_pItemDataVector->end() != it; ++it)
	{
		if (!xr_strcmp((*it).id, str_id))
			break;
	}

	if (m_pItemDataVector->end() == it)
	{
		VERIFY3(no_assert, "item not found, id", *str_id);
		return NULL;
	}

	return &(*it);
}

TEMPLATE_SPECIALIZATION
const ITEM_DATA* CSINI_IdToIndex::GetByIndex(T_INDEX index, bool no_assert)
{
	if ((size_t)index >= m_pItemDataVector->size())
	{
		if (!no_assert)
			Debug.fatal(DEBUG_INFO, "item by index not found in section %s, line %s", section_name, line_name);
		return NULL;
	}
	return &(m_pItemDataVector->at(index));
}

TEMPLATE_SPECIALIZATION
void CSINI_IdToIndex::DeleteIdToIndexData()
{
	xr_delete(m_pItemDataVector);
}

TEMPLATE_SPECIALIZATION
void CSINI_IdToIndex::InitInternal()
{
	VERIFY(!m_pItemDataVector);
	T_INIT::InitIdToIndex();
	{
		m_pItemDataVector = xr_new<T_VECTOR>();

		VERIFY(section_name);
		VERIFY(line_name);

		LPCSTR cfgRecord = pSettings->r_string(section_name, line_name);
		VERIFY(cfgRecord);
		u32 count = _GetItemCount(cfgRecord);
		LoadItemData<ITEM_REC_NUM>(count, cfgRecord);
	}
}


#undef TEMPLATE_SPECIALIZATION
