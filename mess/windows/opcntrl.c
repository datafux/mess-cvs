//============================================================
//
//	opcntrl.c - Code for handling option resolutions in
//	Win32 controls
//
//	This code was factored out of menu.c when Windows Imgtool
//	started to have dynamic controls
//
//============================================================

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <assert.h>

#include "opcntrl.h"
#include "strconv.h"


static const TCHAR guide_prop[] = TEXT("opcntrl_guide");
static const TCHAR spec_prop[] = TEXT("opcntrl_optspec");
static const TCHAR value_prop[] = TEXT("opcntrl_value");



static int get_option_count(const struct OptionGuide *guide,
	const char *optspec)
{
	struct OptionRange ranges[128];
	int count = 0, i;

	option_resolution_listranges(optspec, guide->parameter,
		ranges, sizeof(ranges) / sizeof(ranges[0]));

	for (i = 0; ranges[i].min >= 0; i++)
		count += ranges[i].max - ranges[i].min + 1;
	return count;
}



static BOOL prepare_combobox(HWND control, const struct OptionGuide *guide,
	const char *optspec)
{
	struct OptionRange ranges[128];
	int default_value, default_index, current_index, option_count;
	int i, j;
	BOOL has_option;
	TCHAR buf1[256];
	TCHAR buf2[256];

	SendMessage(control, CB_GETLBTEXT, SendMessage(control, CB_GETCURSEL, 0, 0), (LPARAM) buf1);
	SendMessage(control, CB_RESETCONTENT, 0, 0);
	has_option = guide && optspec;

	if (has_option)
	{
		assert(guide->option_type == OPTIONTYPE_INT);

		option_resolution_listranges(optspec, guide->parameter,
			ranges, sizeof(ranges) / sizeof(ranges[0]));
		option_resolution_getdefault(optspec, guide->parameter, &default_value);

		option_count = 0;
		default_index = -1;
		current_index = -1;

		for (i = 0; ranges[i].min >= 0; i++)
		{
			for (j = ranges[i].min; j <= ranges[i].max; j++)
			{
				_sntprintf(buf2, sizeof(buf2) / sizeof(buf2[0]), TEXT("%d"), j);
				SendMessage(control, CB_ADDSTRING, 0, (LPARAM) buf2);
				SendMessage(control, CB_SETITEMDATA, option_count, j);

				if (j == default_value)
					default_index = option_count;
				if (!strcmp(buf1, buf2))
					current_index = option_count;
				option_count++;
			}
		}
		
		// if there is only one option, it is effectively disabled
		if (option_count <= 1)
			has_option = FALSE;

		if (current_index >= 0)
			SendMessage(control, CB_SETCURSEL, current_index, 0);
		else if (default_index >= 0)
			SendMessage(control, CB_SETCURSEL, default_index, 0);
	}
	else
	{
		// this item is non applicable
		SendMessage(control, CB_ADDSTRING, 0, (LPARAM) TEXT("N/A"));
		SendMessage(control, CB_SETCURSEL, 0, 0);
	}
	EnableWindow(control, has_option);
	return TRUE;
}



static BOOL check_combobox(HWND control)
{
	return TRUE;
}



static BOOL prepare_editbox(HWND control, const struct OptionGuide *guide,
	const char *optspec)
{
	optreserr_t err;
	TCHAR buf[32];
	int val, has_option, option_count;

	has_option = guide && optspec;

	if (has_option)
	{
		err = option_resolution_getdefault(optspec, guide->parameter, &val);
		if (err)
			has_option = FALSE;
	}

	if (has_option)
	{
		_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), "%d", val);

		option_count = get_option_count(guide, optspec);
		if (option_count <= 1)
			has_option = FALSE;
	}
	else
	{
		buf[0] = '\0';
	}
	SetWindowText(control, buf);
	EnableWindow(control, has_option);
	return TRUE;
}



static BOOL check_editbox(HWND control)
{
	TCHAR buf[256];
	const struct OptionGuide *guide;
	const char *optspec;
	optreserr_t err;
	HANDLE h;
	int val;

	guide = (const struct OptionGuide *) GetProp(control, guide_prop);
	optspec = (const char *) GetProp(control, spec_prop);

	GetWindowText(control, buf, sizeof(buf) / sizeof(buf[0]));

	switch(guide->option_type)
	{
		case OPTIONTYPE_INT:
			val = atoi(buf);
			err = option_resolution_isvalidvalue(optspec, guide->parameter, val);
			if (err)
			{
				h = GetProp(control, value_prop);
				val = (int) h;
				_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), TEXT("%d"), val);
				SetWindowText(control, buf);
			}
			else
			{
				SetProp(control, value_prop, (HANDLE) val);
			}
			break;

		default:
			err = OPTIONRESOLTUION_ERROR_INTERNAL;
			break;
	}
	return (err != OPTIONRESOLUTION_ERROR_SUCCESS);
}



BOOL win_prepare_option_control(HWND control, const struct OptionGuide *guide,
	const char *optspec)
{
	BOOL rc = FALSE;
	TCHAR class_name[32];

	SetProp(control, guide_prop, (HANDLE) guide);
	SetProp(control, spec_prop, (HANDLE) optspec);
	GetClassName(control, class_name, sizeof(class_name)
		/ sizeof(class_name[0]));

	if (!_tcsicmp(class_name, TEXT("ComboBox")))
		rc = prepare_combobox(control, guide, optspec);
	else if (!_tcsicmp(class_name, TEXT("Edit")))
		rc = prepare_editbox(control, guide, optspec);

	return rc;
}



BOOL win_check_option_control(HWND control)
{
	BOOL rc = FALSE;
	TCHAR class_name[32];

	GetClassName(control, class_name, sizeof(class_name)
		/ sizeof(class_name[0]));

	if (!_tcsicmp(class_name, TEXT("ComboBox")))
		rc = check_combobox(control);
	else if (!_tcsicmp(class_name, TEXT("Edit")))
		rc = check_editbox(control);

	return rc;
}



BOOL win_adjust_option_control(HWND control, int delta)
{
	const struct OptionGuide *guide;
	const char *optspec;
	struct OptionRange ranges[128];
	TCHAR buf[64];
	int val, original_val, i;
	BOOL changed = FALSE;

	guide = (const struct OptionGuide *) GetProp(control, guide_prop);
	optspec = (const char *) GetProp(control, spec_prop);

	assert(guide->option_type == OPTIONTYPE_INT);

	if (delta == 0)
		return TRUE;

	option_resolution_listranges(optspec, guide->parameter,
		ranges, sizeof(ranges) / sizeof(ranges[0]));

	GetWindowText(control, buf, sizeof(buf) / sizeof(buf[0]));
	original_val = atoi(buf);
	val = original_val + delta;

	for (i = 0; ranges[i].min >= 0; i++)
	{
		if (ranges[i].min > val)
		{
			if ((delta < 0) && (i > 0))
				val = ranges[i-1].max;
			else
				val = ranges[i].min;
			changed = TRUE;
			break;
		}
	}
	if (!changed && (i > 0) && (ranges[i-1].max < val))
		val = ranges[i-1].max;

	if (val != original_val)
	{
		_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), TEXT("%d"), val);
		SetWindowText(control, buf);
	}
	return TRUE;
}



optreserr_t win_add_resolution_parameter(HWND control, option_resolution *resolution)
{
	const struct OptionGuide *guide;
	TCHAR buf[256];
	optreserr_t err;

	if (!GetWindowText(control, buf, sizeof(buf) / sizeof(buf[0])))
		return OPTIONRESOLTUION_ERROR_INTERNAL;

	guide = (const struct OptionGuide *) GetProp(control, guide_prop);
	if (!guide)
		return OPTIONRESOLTUION_ERROR_INTERNAL;

	err = option_resolution_add_param(resolution, guide->identifier, U2T(buf));
	if (err)
		return err;

	return OPTIONRESOLUTION_ERROR_SUCCESS;
}


