// DREAMKAS.CPP
// Copyright (c) A.Sobolev 2018
// @codepage UTF-8
// Интерфейс с кассовым порталом DreamKas
//
#include <pp.h>
#pragma hdrstop

class ACS_DREAMKAS : public PPAsyncCashSession {
public:
	SLAPI  ACS_DREAMKAS(PPID id) : PPAsyncCashSession(id)
	{
	}
	virtual int SLAPI ExportData(int updOnly);
	virtual int SLAPI GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int SLAPI ImportSession(int);
	virtual int SLAPI FinishImportSession(PPIDArray *);
	virtual int SLAPI SetGoodsRestLoadFlag(int updOnly);
protected:
	PPID   StatID;
private:
	int    SLAPI ConvertWareList(const char * pImpPath);

	DateRange ChkRepPeriod;
	PPIDArray LogNumList;
	PPIDArray SessAry;
	SString PathRpt;
	SString PathFlag;
	SString PathGoods;
	SString PathGoodsFlag;
	int    UseAltImport;
	int    CrdCardAsDsc;
	int    SkipExportingDiscountSchemes;
	int    ImpExpTimeout;
	int    ImportDelay;
	StringSet ImpPaths;
	StringSet ExpPaths;
	PPAsyncCashNode Acn;
	SString   ImportedFiles;
};

class CM_DREAMKAS : public PPCashMachine {
public:
	SLAPI CM_DREAMKAS(PPID cashID) : PPCashMachine(cashID)
	{
	}
	PPAsyncCashSession * SLAPI AsyncInterface() { return new ACS_DREAMKAS(NodeID); }
};

REGISTER_CMT(DREAMKAS, 0, 1);

//virtual 
int SLAPI ACS_DREAMKAS::ExportData(int updOnly)
{
	int    ok = 1;
	//
	ScURL c;
	const char * p_url_base = "https://kabinet.dreamkas.ru/api";
	const char * p_user = "";
	const char * p_password = "";

	int    next_barcode = 0;
	uint   i;
	LAssocArray  grp_n_level_ary;
	SString   f_str;
	SString   tail;
	SString   temp_buf;
	SString   email_subj;
	SString   path_goods;
	SString   path_flag;
	//
	PPID      gc_alc_id = 0;
	SString   gc_alc_code; // Код класса товаров, относящихся к алкоголю
	PPIDArray alc_goods_list;
	//
	//PPUnit    unit_rec;
	//PPObjUnit unit_obj;
	PPObjQuotKind qk_obj;
	PPObjSCardSeries scs_obj;
	PPSCardSeries ser_rec;
	PPSCardSerPacket scs_pack;
	PPObjGoods goods_obj;
	PPObjGoodsClass gc_obj;
	PPAsyncCashNode cn_data;
	LAssocArray  scard_quot_list;
	PPIDArray retail_quot_list;
	PPIDArray scard_series_list;
	BitArray used_retail_quot;
	PPIniFile ini_file;
	const  int check_dig = BIN(GetGoodsCfg().Flags & GCF_BCCHKDIG);
	THROW(GetNodeData(&cn_data) > 0);
	//
	// Извлечем информацию о классе алкогольного товара
	//
	if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_GOODSCLASSALC, temp_buf) > 0 && temp_buf.NotEmptyS()) {
		StringSet ss(',', temp_buf);
		ss.get(&(i = 0), temp_buf.Z());
		if(gc_obj.SearchBySymb(temp_buf, &gc_alc_id) > 0) {
			PPGdsClsPacket gc_pack;
			if(gc_obj.Fetch(gc_alc_id, &gc_pack) > 0) {
				gc_alc_id = gc_pack.Rec.ID;
				(temp_buf = gc_pack.Rec.Symb).Strip();
				//
				// Если код товарного класса числовой, то используем для загрузки его, иначе - идентификатор
				// (фронтол требует цифрового кода, но использование идентификатора не желательно из-за возможного разнобоя между разделами БД).
				//
				if(temp_buf.ToLong() > 0)
					gc_alc_code = temp_buf;
				else
					gc_alc_code.Z().Cat(gc_alc_id);
			}
		}
	}
	// }
	THROW_MEM(SETIFZ(P_Dls, new DeviceLoadingStat));
	P_Dls->StartLoading(&StatID, dvctCashs, NodeID, 1);
	PPWait(1);
	{
		qk_obj.GetRetailQuotList(ZERODATETIME, &retail_quot_list, 0);
		used_retail_quot.insertN(0, retail_quot_list.getCount());
	}
	if(updOnly || (Flags & PPACSF_LOADRESTWOSALES)) {
	}
	else {
	}
	{
		json_t jdoc(json_t::tOBJECT);
		long   acgif = 0;
		if(updOnly) {
			acgif |= ACGIF_UPDATEDONLY;
			if(updOnly == 2)
				acgif |= ACGIF_REDOSINCEDLS;
		}
		if(cn_data.ExtFlags & CASHFX_EXPLOCPRNASSOC)
			acgif |= ACGIF_INITLOCPRN;
		AsyncCashGoodsIterator goods_iter(NodeID, acgif, SinceDlsID, P_Dls);
		{
			PPID   prev_goods_id = 0;
			AsyncCashGoodsInfo gds_info;
			PPIDArray rmv_goods_list;
			PrcssrAlcReport::GoodsItem agi;
			for(i = 0; i < retail_quot_list.getCount(); i++) {
				gds_info.QuotList.Add(retail_quot_list.get(i), 0, 1);
			}
			json_t * p_iter_ary = new json_t(json_t::tARRAY);
			while(goods_iter.Next(&gds_info) > 0) {
				if(gds_info.GoodsFlags & GF_PASSIV && cn_data.ExtFlags & CASHFX_RMVPASSIVEGOODS && gds_info.Rest <= 0.0) {
					rmv_goods_list.addUnique(gds_info.ID);
				}
				else {
	   				if(gds_info.ID != prev_goods_id) {
						long   level = 0;
						PPID   dscnt_scheme_id = 0;
						if(prev_goods_id) {
							json_t * p_iter_obj = new json_t(json_t::tOBJECT);
							json_insert_pair_into_object(p_iter_obj, "id", json_new_string(temp_buf.Z().Cat(gds_info.Uuid, S_GUID::fmtIDL)));
							json_insert_pair_into_object(p_iter_obj, "name", json_new_string(temp_buf.Z().Cat(gds_info.Name).Transf(CTRANSF_INNER_TO_UTF8)));
							json_insert_pair_into_object(p_iter_obj, "type", json_new_string("COUNTABLE"));
							json_insert_pair_into_object(p_iter_obj, "departmentId", json_new_number(temp_buf.Z().Cat(gds_info.DivN)));
							json_insert_pair_into_object(p_iter_obj, "quantity", json_new_number(temp_buf.Z().Cat(1000)));
							json_insert_pair_into_object(p_iter_obj, "price", json_new_number(temp_buf.Z().Cat((long)(gds_info.Price * 100.0))));
						}
						if(gc_alc_id && gc_alc_code.NotEmpty() && gds_info.GdsClsID == gc_alc_id) {
							alc_goods_list.add(gds_info.ID);
						}
						next_barcode = 0;
						if(goods_iter.GetAlcoGoodsExtension(gds_info.ID, 0, agi) > 0) {
						}
						else {
						}
					}
					const size_t bclen = sstrlen(gds_info.BarCode);
					if(bclen) {
						gds_info.AdjustBarcode(check_dig);
						int    wp = GetGoodsCfg().IsWghtPrefix(gds_info.BarCode);
						if(wp == 1)
							STRNSCPY(gds_info.BarCode, gds_info.BarCode+sstrlen(GetGoodsCfg().WghtPrefix));
						else if(wp == 2)
							STRNSCPY(gds_info.BarCode, gds_info.BarCode+sstrlen(GetGoodsCfg().WghtCntPrefix));
						else
							AddCheckDigToBarcode(gds_info.BarCode);
						if(next_barcode) {
							;
						}
						next_barcode = 1;
					}
				}
	   			prev_goods_id = gds_info.ID;
				PPWaitPercent(goods_iter.GetIterCounter());
			}
			if(prev_goods_id) {
				;
			}
			//
			// Список товаров на удаление.
			//
			if(rmv_goods_list.getCount()) {
				for(i = 0; i < rmv_goods_list.getCount(); i++) {
					const PPID goods_id_to_remove = rmv_goods_list.get(i);
				}
			}
			//
			// Список алкогольных товаров
			//
			if(alc_goods_list.getCount()) {
				alc_goods_list.sortAndUndup();
				for(i = 0; i < alc_goods_list.getCount(); i++) {
					;
				}
			}
		}
	}
	PPWait(0);
	PPWait(1);
	//
	// Здесь отправить данные на сервер
	//
	if(StatID)
		P_Dls->FinishLoading(StatID, 1, 1);
	CATCHZOK
	PPWait(0);
	return ok;
}

//virtual 
int SLAPI ACS_DREAMKAS::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd)
{
	return -1;
}
//virtual 
int SLAPI ACS_DREAMKAS::ImportSession(int)
{
	return -1;
}
//virtual 
int SLAPI ACS_DREAMKAS::FinishImportSession(PPIDArray *)
{
	return -1;
}
//virtual 
int SLAPI ACS_DREAMKAS::SetGoodsRestLoadFlag(int updOnly)
{
	return -1;
}