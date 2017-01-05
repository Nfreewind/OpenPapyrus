// GTAXDLG.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2005, 2007, 2016
//
#include <pp.h>
#pragma hdrstop

class GoodsTaxDialog : public TDialog {
public:
	GoodsTaxDialog(uint dlgID) : TDialog(dlgID)
	{
		SetupCalCtrl(CTLCAL_GDSTAX_PERIOD, this, CTL_GDSTAX_PERIOD, 1);
	}
	int    setDTS(const PPGoodsTaxPacket *);
	int    getDTS(PPGoodsTaxPacket *);
	int    setEntry(const PPGoodsTaxEntry *);
	int    getEntry(PPGoodsTaxEntry *);
private:
	DECL_HANDLE_EVENT;
	void   editList();
	PPGoodsTaxPacket Data;
	PPGoodsTaxEntry  Entry;
	PPObjGoodsTax GTxObj;
};

IMPL_HANDLE_EVENT(GoodsTaxDialog)
{
	ushort v;
	TDialog::handleEvent(event);
	if(TVCOMMAND)
		if(TVCMD == cmGoodsTaxList) {
			getCtrlData(CTL_GDSTAX_FLAGS, &(v = 0));
			if(v & 0x01)
				editList();
			clearEvent(event);
		}
		else if(event.isClusterClk(CTL_GDSTAX_FLAGS)) {
			getCtrlData(CTL_GDSTAX_FLAGS, &(v = 0));
			enableCommand(cmGoodsTaxList, BIN(v & 0x01));
			clearEvent(event);
		}
}

int GoodsTaxDialog::setEntry(const PPGoodsTaxEntry * pEntry)
{
	Entry = *pEntry;
	char   str[32];
	if(Entry.Flags & GTAXF_ENTRY) {
		SetPeriodInput(this, CTL_GDSTAX_PERIOD, &Entry.Period);
		SetupOprKindCombo(this, CTLSEL_GDSTAX_OP, Entry.OpID, 0, 0, 0);
	}
	setCtrlData(CTL_GDSTAX_VAT,     Entry.FormatVAT(str, sizeof(str)));
	setCtrlData(CTL_GDSTAX_EXCISE,  Entry.FormatExcise(str, sizeof(str)));
	setCtrlData(CTL_GDSTAX_STAX,    Entry.FormatSTax(str, sizeof(str)));
	GTxObj.FormatOrder(Entry.Order, Entry.UnionVect, str, sizeof(str));
	setCtrlData(CTL_GDSTAX_ORDER, str);
	return 1;
}

int GoodsTaxDialog::getEntry(PPGoodsTaxEntry * pEntry)
{
	int    ok = 1;
	char   str[32];
	double rv;
	if(Entry.Flags & GTAXF_ENTRY) {
		GetPeriodInput(this, CTL_GDSTAX_PERIOD, &Entry.Period);
		getCtrlData(CTLSEL_GDSTAX_OP, &Entry.OpID);
	}
	getCtrlData(CTL_GDSTAX_VAT, str);
	strtodoub(str, &rv);
	Entry.VAT = R0i(rv * 100L);
	Entry.Flags &= ~GTAXF_ABSEXCISE;
	getCtrlData(CTL_GDSTAX_EXCISE, str);
	if(*strip(str)) {
		char * p_dollar = strchr(str, '$');
		if(p_dollar) {
			*p_dollar = 0;
			Entry.Flags |= GTAXF_ABSEXCISE;
		}
		strtodoub(str, &rv);
		Entry.Excise = R0i(rv * 100L);
	}
	else
		Entry.Excise = 0;
	if(Entry.Excise == 0)
		Entry.Flags &= ~GTAXF_ABSEXCISE;
	getCtrlData(CTL_GDSTAX_STAX, str);
	strtodoub(str, &rv);
	Entry.SalesTax = R0i(rv * 100L);
	getCtrlData(CTL_GDSTAX_ORDER, str);
	if(GTxObj.StrToOrder(str, &Entry.Order, &Entry.UnionVect)) {
		*pEntry = Entry;
	}
	else {
		selectCtrl(CTL_GDSTAX_ORDER);
		ok = (PPError(PPERR_INVEXPR, str), 0); // @TODO (err code)
	}
	return ok;
}

int GoodsTaxDialog::setDTS(const PPGoodsTaxPacket * pData)
{
	RVALUEPTR(Data, pData);
	PPGoodsTaxEntry entry;
	setCtrlData(CTL_GDSTAX_NAME, Data.Rec.Name);
	setCtrlData(CTL_GDSTAX_SYMB, Data.Rec.Symb);
	setCtrlData(CTL_GDSTAX_ID,  &Data.Rec.ID);
	AddClusterAssoc(CTL_GDSTAX_FLAGS, 0, GTAXF_USELIST);
	SetClusterData(CTL_GDSTAX_FLAGS, Data.Rec.Flags);
	enableCommand(cmGoodsTaxList, BIN(Data.Rec.Flags & GTAXF_USELIST));
	AddClusterAssoc(CTL_GDSTAX_NOLOTEXCISE, 0, GTAXF_NOLOTEXCISE);
	SetClusterData(CTL_GDSTAX_NOLOTEXCISE, Data.Rec.Flags);
	Data.Rec.ToEntry(&entry);
	return setEntry(&entry);
}

int GoodsTaxDialog::getDTS(PPGoodsTaxPacket * pData)
{
	int    ok = 1;
	PPGoodsTaxEntry entry;
	getCtrlData(CTL_GDSTAX_NAME, Data.Rec.Name);
	getCtrlData(CTL_GDSTAX_SYMB, Data.Rec.Symb);
	if(*strip(Data.Rec.Name) == 0)
		GTxObj.GetDefaultName(&Data.Rec, Data.Rec.Name, sizeof(Data.Rec.Name));
	if(Data.Rec.ID == 0)
		getCtrlData(CTL_GDSTAX_ID, &Data.Rec.ID);
	MEMSZERO(entry);
	if(getEntry(&entry)) {
		Data.Rec.FromEntry(&entry);
		GetClusterData(CTL_GDSTAX_FLAGS, &Data.Rec.Flags);
		GetClusterData(CTL_GDSTAX_NOLOTEXCISE, &Data.Rec.Flags);
		ASSIGN_PTR(pData, Data);
		return 1;
	}
	else
		ok = 0;
	return ok;
}

class GoodsTaxListDialog : public PPListDialog {
public:
	GoodsTaxListDialog() : PPListDialog(DLG_GDSTAXLST, CTL_GDSTAXLST_LIST)
	{
		disableCtrl(CTL_GDSTAXLST_NAME, 1);
		updateList(-1);
	}
	int    setDTS(const PPGoodsTaxPacket *);
	int    getDTS(PPGoodsTaxPacket *);
private:
	DECL_HANDLE_EVENT;
	virtual int  setupList();
	virtual int  addItem(long * pPos, long * pID);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	int    editItemDialog(int, PPGoodsTaxEntry *);
	void   addBySample();

	PPGoodsTaxPacket Data;
};

IMPL_HANDLE_EVENT(GoodsTaxListDialog)
{
	PPListDialog::handleEvent(event);
	if(TVKEYDOWN && TVKEY == kbAltF2) {
		addBySample();
		clearEvent(event);
	}
}

int GoodsTaxListDialog::setDTS(const PPGoodsTaxPacket * pData)
{
	Data = *pData;

	setCtrlData(CTL_GDSTAXLST_NAME, Data.Rec.Name);
	setCtrlData(CTL_GDSTAXLST_ID, &Data.Rec.ID);
	disableCtrl(CTL_GDSTAXLST_ID, 1);
	updateList(-1);
	return 1;
}

int GoodsTaxListDialog::getDTS(PPGoodsTaxPacket * pData)
{
	int    ok = 1;
	getCtrlData(CTL_GDSTAXLST_NAME, Data.Rec.Name);
	*pData = Data;
	return ok;
}

int GoodsTaxListDialog::setupList()
{
	PPGoodsTaxEntry * p_item = 0;
	for(uint i = 0; Data.enumItems(&i, (void**)&p_item);) {
		StringSet ss(SLBColumnDelim);
		char   sub[64];
		ss.add(periodfmt(&p_item->Period, sub));
		if(p_item->OpID)
			GetOpName(p_item->OpID, sub, sizeof(sub));
		else
			sub[0] = 0;
		ss.add(sub);
		ss.add(p_item->FormatExcise(sub, sizeof(sub)));
		ss.add(p_item->FormatVAT(sub, sizeof(sub)));
		ss.add(p_item->FormatSTax(sub, sizeof(sub)));
		if(!addStringToList(i, ss.getBuf()))
			return 0;
	}
	return 1;
}

int GoodsTaxListDialog::delItem(long pos, long)
{
	if(pos >= 0) {
		Data.atFree((uint)pos);
		return 1;
	}
	return -1;
}

int GoodsTaxListDialog::editItemDialog(int pos, PPGoodsTaxEntry * pEntry)
{
	int    ok = -1;
	GoodsTaxDialog * dlg = new GoodsTaxDialog(DLG_GDSTAXENTRY);
	if(CheckDialogPtr(&dlg, 1)) {
		dlg->setEntry(pEntry);
		dlg->setCtrlData(CTL_GDSTAX_NAME, Data.Rec.Name);
		dlg->disableCtrl(CTL_GDSTAX_NAME, 1);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
			if(dlg->getEntry(pEntry) && Data.PutEntry(pos, pEntry))
				ok = valid_data = 1;
			else
				PPError();
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}

void GoodsTaxListDialog::addBySample()
{
	long   p, i;
	if(getCurItem(&p, &i)) {
		PPGoodsTaxEntry item = *(PPGoodsTaxEntry*)Data.at((uint)p);
		if(editItemDialog(-1, &item) > 0)
			updateList(-1);
	}
}

int GoodsTaxListDialog::addItem(long * pPos, long * pID)
{
	PPGoodsTaxEntry item;
	MEMSZERO(item);
	Data.Rec.ToEntry(&item);
	item.Flags |= GTAXF_ENTRY;
	item.Flags &= ~GTAXF_USELIST;
	if(editItemDialog(-1, &item) > 0) {
		ASSIGN_PTR(pPos, Data.getCount()-1);
		ASSIGN_PTR(pID, Data.getCount());
		return 1;
	}
	else
		return -1;
}

int GoodsTaxListDialog::editItem(long pos, long)
{
	if(pos >= 0 && pos < (long)Data.getCount()) {
		PPGoodsTaxEntry item = *(PPGoodsTaxEntry*)Data.at((uint)pos);
		if(editItemDialog((int)pos, &item) > 0)
			return 1;
	}
	return -1;
}

void GoodsTaxDialog::editList()
{
	GoodsTaxListDialog * dlg = new GoodsTaxListDialog();
	if(CheckDialogPtr(&dlg, 1)) {
		dlg->setDTS(&Data);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
			if(dlg->getDTS(&Data))
				valid_data = 1;
	}
	delete dlg;
}

int SLAPI PPObjGoodsTax::AddBySample(PPID * pID, long sampleID)
{
	int    r = cmCancel;
	PPGoodsTaxPacket pack;
	GoodsTaxDialog * dlg = 0;
	if(GetPacket(sampleID, &pack) > 0) {
		PPGoodsTaxEntry * p_entry;
		pack.Rec.ID = 0;
		for(uint i = 0; pack.enumItems(&i, (void **)&p_entry);)
			p_entry->TaxGrpID = 0;
		if(CheckDialogPtr(&(dlg = new GoodsTaxDialog(DLG_GDSTAX)), 1)) {
			dlg->setDTS(&pack);
			for(int valid_data = 0; !valid_data && (r = ExecView(dlg)) == cmOK;)
				if(dlg->getDTS(&pack))
					if(!CheckName(*pID, pack.Rec.Name, 0))
						dlg->selectCtrl(CTL_GDSTAX_NAME);
					else if(PutPacket(pID, &pack, 1)) {
						Dirty(*pID);
						valid_data = 1;
					}
		}
	}
	delete dlg;
	return r;
}

int SLAPI PPObjGoodsTax::Edit(PPID * pID, void * extraPtr)
{
	int    r = cmCancel, valid_data = 0, is_new = 0;
	PPGoodsTaxPacket pack;
	GoodsTaxDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new GoodsTaxDialog(DLG_GDSTAX))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		THROW(is_new || CheckRights(PPR_MOD));
		if(dlg->getDTS(&pack))
			if(CheckDupName(*pID, pack.Rec.Name) &&
				ref->CheckUniqueSymb(Obj, *pID, pack.Rec.Symb, offsetof(PPGoodsTax, Symb))) {
				if(PutPacket(pID, &pack, 1)) {
					Dirty(*pID);
					valid_data = 1;
				}
			}
			else
				PPErrorByDialog(dlg, CTL_GDSTAX_NAME, -1);
	}
	CATCH
		r = PPErrorZ();
	ENDCATCH
	delete dlg;
#ifdef TEST_GTAX
	Test(*pID);
#endif
	return r;
}
