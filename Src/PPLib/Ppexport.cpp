// PPEXPORT.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2010, 2011, 2012, 2015, 2016
// @codepage windows-1251
// ������/������� ������
//
#include <pp.h>
#pragma hdrstop

//char * _ExportNames[] = {
	//"MONARCH",
	//"LP15",
	//"GLABEL"
//};

int SLAPI ProcessExportJob(const char * pJobName)
{
	int    ok = 0;
	PPID   cash_id = 0;
	PPObjCashNode cn_obj;
	PPCashNode cn_rec;
	if(cn_obj.SearchByName(pJobName, &cash_id, &cn_rec) > 0) {
		if(PPCashMachine::IsAsyncCMT(cn_rec.CashType)) {
			PPCashMachine * cm = PPCashMachine::CreateInstance(cash_id);
			if(cm) {
				ok = cm->AsyncOpenSession(0, 0);
				delete cm;
			}
		}
	}
   	return ok;
}

int SLAPI ImportGeoCity(const char * pPath);

int SLAPI ProcessImportJob(const char * pJobName)
{
	int    ok = 0;
	SString job_name = pJobName;
	if(job_name.CmpPrefix("GEOCITY", 1) == 0) {
		SString left, right;
		if(job_name.Divide('@', left, right) > 0)
			ImportGeoCity(right);
	}
	else {
		PPID   cash_id = 0;
		PPObjCashNode cn_obj;
		PPCashNode cn_rec;
		if(cn_obj.SearchByName(job_name, &cash_id, &cn_rec) > 0)
			if(PPCashMachine::IsAsyncCMT(cn_rec.CashType)) {
				PPCashMachine * cm = PPCashMachine::CreateInstance(cash_id);
				if(cm) {
					ok = cm->AsyncCloseSession();
					if(!ok)
					   	PPError();
					delete cm;
				}
			}
	}
	return ok;
}
//
//
//
SLAPI PPDbTableXmlExporter::BaseParam::BaseParam(uint32 sign)
{
	Sign = sign;
	Flags = 0;
	RefDbID = 0;
}

int SLAPI PPDbTableXmlExporter::BaseParam::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, Sign, rBuf));
	THROW(pSCtx->Serialize(dir, Flags, rBuf));
	THROW(pSCtx->Serialize(dir, RefDbID, rBuf));
	THROW(pSCtx->Serialize(dir, FileName, rBuf));
	CATCHZOK
	return ok;
}

SLAPI PPDbTableXmlExporter::PPDbTableXmlExporter()
{
}

//virtual
SLAPI PPDbTableXmlExporter::~PPDbTableXmlExporter()
{
}

int SLAPI PPDbTableXmlExporter::Run(const char * pOutFileName)
{
	int    ok = 1;
	const  long preserve_dt_def_fmt = SLS.GetConstTLA().TxtDateFmt_;
	xmlTextWriterPtr writer = 0;
	PPWait(1);
	DBTable * p_t = Init();
	THROW(p_t);
	THROW(writer = xmlNewTextWriterFilename(pOutFileName, 0));
	{
		SString temp_buf, fld_name;
		const BNFieldList & r_fl = p_t->fields;
		xmlTextWriterSetIndent(writer, 1);
		xmlTextWriterSetIndentString(writer, (const xmlChar*)"\t");
		xmlTextWriterStartDocument(writer, 0, "utf-8", 0);
		// XMLWriteSpecSymbEntities(writer);
		{
			SLS.GetTLA().TxtDateFmt_ = DATF_DMY|DATF_CENTURY;
			(temp_buf = p_t->tableName).ToUtf8();
			SXml::WNode n_tbl(writer, temp_buf);
			DS.GetVersion().ToStr(temp_buf = 0);
			n_tbl.PutAttrib("version", temp_buf);
			{
				CurDict->GetDbSymb(temp_buf);
				temp_buf.ToUtf8();
				n_tbl.PutAttrib("dbsymb", temp_buf);
			}
			{
				S_GUID uuid;
				CurDict->GetDbUUID(&uuid);
				uuid.ToStr(S_GUID::fmtIDL, temp_buf);
				n_tbl.PutAttrib("dbuuid", temp_buf);
			}
			if(Cntr.GetTotal()) {
				(temp_buf = 0).Cat(Cntr.GetTotal());
				n_tbl.PutAttrib("count", temp_buf);
			}
			while(Next() > 0) {
				SXml::WNode n_rec(writer, "record");
				for(uint i = 0; i < r_fl.getCount(); i++) {
					char   _buf[1024];
					const BNField & f = r_fl[i];
					{
						(fld_name = f.Name).ToUtf8();
						f.putValueToString(p_t->getDataBuf(), _buf);
						(temp_buf = _buf).Transf(CTRANSF_INNER_TO_UTF8);
						XMLReplaceSpecSymb(temp_buf, "&<>\'");
						SXml::WNode(writer, fld_name, temp_buf);
					}
				}
				if(Cntr.GetTotal()) {
					PPWaitPercent(Cntr);
				}
			}
		}
	}
	CATCHZOK
	PPWait(0);
	xmlFreeTextWriter(writer);
	SLS.GetTLA().TxtDateFmt_ = preserve_dt_def_fmt;
	return ok;
}
//
//
//
PPDbTableXmlExportParam_TrfrBill::PPDbTableXmlExportParam_TrfrBill() : PPDbTableXmlExporter::BaseParam(0xEF00BC02)
{
	Period.SetZero();
}

int SLAPI PPDbTableXmlExportParam_TrfrBill::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(PPDbTableXmlExporter::BaseParam::Serialize(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, Period.low, rBuf));
	THROW(pSCtx->Serialize(dir, Period.upp, rBuf));
	CATCHZOK
	return ok;
}

#define GRP_BROWSE 1

class DbTableXmlExportParamDialog : public TDialog {
public:
	DbTableXmlExportParamDialog(uint dlgId) : TDialog(dlgId), Data(0)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_DBTEXP_PATH, CTL_DBTEXP_PATH, GRP_BROWSE,
			PPTXT_TITLE_SELDBTXMLEXPPATH, 0, FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfAllowNExists);
	}
	int    setDTS(const PPDbTableXmlExporter::BaseParam * pData)
	{
		int    ok = 1;
		if(pData)
			Data = *pData;
		AddClusterAssoc(CTL_DBTEXP_FLAGS, 0, Data.fReplaceIdsBySync);
		SetClusterData(CTL_DBTEXP_FLAGS, Data.Flags);
		SetupPPObjCombo(this, CTLSEL_DBTEXP_REFDBDIV, PPOBJ_DBDIV, Data.RefDbID, 0, 0);
		setCtrlString(CTL_DBTEXP_PATH, Data.FileName);
		disableCtrl(CTLSEL_DBTEXP_REFDBDIV, !BIN(Data.Flags & Data.fReplaceIdsBySync));
		return ok;
	}
	int    getDTS(PPDbTableXmlExporter::BaseParam * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		GetClusterData(CTL_DBTEXP_FLAGS, &Data.Flags);
		getCtrlData(CTLSEL_DBTEXP_REFDBDIV, &Data.RefDbID);
		getCtrlString(sel = CTL_DBTEXP_PATH, Data.FileName);
		THROW_PP(Data.FileName.NotEmptyS(), PPERR_FILENAMENEEDED);
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = PPErrorByDialog(this, sel, -1);
		ENDCATCH
		return ok;
	}
protected:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_DBTEXP_FLAGS)) {
			GetClusterData(CTL_DBTEXP_FLAGS, &Data.Flags);
			disableCtrl(CTLSEL_DBTEXP_REFDBDIV, !BIN(Data.Flags & Data.fReplaceIdsBySync));
		}
		else
			return;
		clearEvent(event);
	}

	PPDbTableXmlExporter::BaseParam Data;
};

#undef GRP_BROWSE

int SLAPI PPDbTableXmlExportParam_TrfrBill::Edit(PPDbTableXmlExportParam_TrfrBill * pData)
{
    int    ok = -1;
    DbTableXmlExportParamDialog * dlg = new DbTableXmlExportParamDialog(DLG_DBTEXPTRFR);
    if(CheckDialogPtr(&dlg, 1)) {
		dlg->SetupCalPeriod(CTLCAL_DBTEXP_PERIOD, CTL_DBTEXP_PERIOD);
		dlg->setDTS(pData);
        SetPeriodInput(dlg, CTL_DBTEXP_PERIOD, &pData->Period);
        dlg->setCtrlString(CTL_DBTEXP_PATH, pData->FileName);
        while(ok < 0 && ExecView(dlg) == cmOK) {
			uint   sel = 0;
			if(dlg->getDTS(pData)) {
				if(!GetPeriodInput(dlg, sel = CTL_DBTEXP_PERIOD, &pData->Period)) {
					PPErrorByDialog(dlg, sel, -1);
				}
				else {
					ok = 1;
				}
			}
        }
    }
    else
		ok = 0;
	delete dlg;
    return ok;
}
//
//
//
SLAPI PPDbTableXmlExporter_Transfer::PPDbTableXmlExporter_Transfer(const PPDbTableXmlExportParam_TrfrBill & rParam) : PPDbTableXmlExporter()
{
	P = rParam;
	P.Period.Actualize(ZERODATE);
	P_T = BillObj ? BillObj->trfr : 0;
	P_Q = 0;
}

//virtual
DBTable * PPDbTableXmlExporter_Transfer::Init()
{
	ZDELETE(P_Q);
	if(P_T) {
		TransferTbl::Key1 k1, k1_;
		MEMSZERO(k1);
		k1.Dt = P.Period.low;
		THROW_MEM(P_Q = new BExtQuery(P_T, 1));
		P_Q->selectAll().where(daterange(P_T->Dt, &P.Period));
		k1_ = k1;
		Cntr.Init(P_Q->countIterations(0, &k1_, spGe));
		THROW(P_Q->initIteration(0, &k1, spGe));
	}
	CATCH
		P_T = 0;
		ZDELETE(P_Q);
	ENDCATCH
	return P_T;
}

//virtual
int PPDbTableXmlExporter_Transfer::Next()
{
	int    ok = P_Q ? P_Q->nextIteration() : 0;
	if(ok > 0)
		Cntr.Increment();
	return ok;
}
//
//
//
SLAPI PPDbTableXmlExporter_Bill::PPDbTableXmlExporter_Bill(const PPDbTableXmlExportParam_TrfrBill & rParam) : PPDbTableXmlExporter()
{
	P = rParam;
	P.Period.Actualize(ZERODATE);
	P_T = BillObj ? BillObj->P_Tbl : 0;
	P_Q = 0;
}

//virtual
DBTable * PPDbTableXmlExporter_Bill::Init()
{
	ZDELETE(P_Q);
	if(P_T) {
		BillTbl::Key1 k1, k1_;
		MEMSZERO(k1);
		k1.Dt = P.Period.low;
		THROW_MEM(P_Q = new BExtQuery(P_T, 1));
		P_Q->selectAll().where(daterange(P_T->Dt, &P.Period));
		k1_ = k1;
		Cntr.Init(P_Q->countIterations(0, &k1_, spGe));
		THROW(P_Q->initIteration(0, &k1, spGe));
	}
	CATCH
		P_T = 0;
		ZDELETE(P_Q);
	ENDCATCH
	return P_T;
}

//virtual
int PPDbTableXmlExporter_Bill::Next()
{
	int    ok = P_Q ? P_Q->nextIteration() : 0;
	if(ok > 0)
		Cntr.Increment();
	return ok;
}

