/******************************************************************************
 *
 * Project:  S-57 Translator
 * Purpose:  Implements OGRS57Layer class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam warmerdam@pobox.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

/**
 * \file
 * Implement OGRS57Layer class.
 */

#include "ogr_s57.h"
#include "gdal/cpl_conv.h"
#include "gdal/cpl_string.h"

/************************************************************************/
/*                            OGRS57Layer()                             */
/*                                                                      */
/*      Note that the OGRS57Layer assumes ownership of the passed       */
/*      OGRFeatureDefn object.                                          */
/************************************************************************/

OGRS57Layer::OGRS57Layer(OGRS57DataSource *poDSIn, OGRFeatureDefn *poDefnIn,
                         int nFeatureCountIn, int nOBJLIn)

{
  poFilterGeom = NULL;

  poDS = poDSIn;

  nFeatureCount = nFeatureCountIn;

  poFeatureDefn = poDefnIn;

  nOBJL = nOBJLIn;

  nNextFEIndex = 0;
  nCurrentModule = -1;

  if (EQUAL(poDefnIn->GetName(), OGRN_VI))
    nRCNM = RCNM_VI;
  else if (EQUAL(poDefnIn->GetName(), OGRN_VC))
    nRCNM = RCNM_VC;
  else if (EQUAL(poDefnIn->GetName(), OGRN_VE))
    nRCNM = RCNM_VE;
  else if (EQUAL(poDefnIn->GetName(), OGRN_VF))
    nRCNM = RCNM_VF;
  else
    nRCNM = 100; /* feature */
}

/************************************************************************/
/*                           ~OGRS57Layer()                           */
/************************************************************************/

OGRS57Layer::~OGRS57Layer()

{
  delete poFeatureDefn;

  if (poFilterGeom != NULL) delete poFilterGeom;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRS57Layer::SetSpatialFilter(OGRGeometry *poGeomIn)

{
  if (poFilterGeom != NULL) {
    delete poFilterGeom;
    poFilterGeom = NULL;
  }

  if (poGeomIn != NULL) poFilterGeom = poGeomIn->clone();

  if (nNextFEIndex != 0 || nCurrentModule != -1) ResetReading();
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRS57Layer::ResetReading()

{
  nNextFEIndex = 0;
  nCurrentModule = -1;
}

/************************************************************************/
/*                      GetNextUnfilteredFeature()                      */
/************************************************************************/

OGRFeature *OGRS57Layer::GetNextUnfilteredFeature()

{
  OGRFeature *poFeature = NULL;

  /* -------------------------------------------------------------------- */
  /*      Are we out of modules to request features from?                 */
  /* -------------------------------------------------------------------- */
  if (nCurrentModule >= poDS->GetModuleCount()) return NULL;

  /* -------------------------------------------------------------------- */
  /*      Set the current position on the current module and fetch a      */
  /*      feature.                                                        */
  /* -------------------------------------------------------------------- */
  S57Reader *poReader = poDS->GetModule(nCurrentModule);

  if (poReader != NULL) {
    poReader->SetNextFEIndex(nNextFEIndex, nRCNM);
    poFeature = poReader->ReadNextFeature(poFeatureDefn);
    nNextFEIndex = poReader->GetNextFEIndex(nRCNM);
  }

  /* -------------------------------------------------------------------- */
  /*      If we didn't get a feature we need to move onto the next file.  */
  /* -------------------------------------------------------------------- */
  if (poFeature == NULL) {
    nCurrentModule++;
    poReader = poDS->GetModule(nCurrentModule);

    if (poReader != NULL && poReader->GetModule() == NULL) {
      if (!poReader->Open(FALSE)) return NULL;
    }

    return GetNextUnfilteredFeature();
  } else {
    if (poFeature->GetGeometryRef() != NULL)
      poFeature->GetGeometryRef()->assignSpatialReference(GetSpatialRef());
  }

  return poFeature;
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRS57Layer::GetNextFeature()

{
  OGRFeature *poFeature = NULL;

  /* -------------------------------------------------------------------- */
  /*      Read features till we find one that satisfies our current       */
  /*      spatial criteria.                                               */
  /* -------------------------------------------------------------------- */
  while (TRUE) {
    poFeature = GetNextUnfilteredFeature();
    if (poFeature == NULL) break;
    /*
            if( (poFilterGeom == NULL
                 || poFeature->GetGeometryRef() == NULL
                 || poFilterGeom->Intersect( poFeature->GetGeometryRef() ) )
                && (m_poAttrQuery == NULL
                    || m_poAttrQuery->Evaluate( poFeature )) )
                break;
      */
    delete poFeature;
  }

  return poFeature;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRS57Layer::TestCapability(const char *pszCap)

{
  if (EQUAL(pszCap, OLCRandomRead))
    return FALSE;

  else if (EQUAL(pszCap, OLCSequentialWrite))
    return TRUE;

  else if (EQUAL(pszCap, OLCRandomWrite))
    return FALSE;

  else if (EQUAL(pszCap, OLCFastFeatureCount))
    return TRUE;

  else if (EQUAL(pszCap, OLCFastGetExtent)) {
    OGREnvelope oEnvelope;

    return GetExtent(&oEnvelope, FALSE) == OGRERR_NONE;
  } else if (EQUAL(pszCap, OLCFastSpatialFilter))
    return FALSE;

  else
    return FALSE;
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/************************************************************************/

OGRSpatialReference *OGRS57Layer::GetSpatialRef()

{
  return poDS->GetSpatialRef();
}

/************************************************************************/
/*                             GetExtent()                              */
/************************************************************************/

OGRErr OGRS57Layer::GetExtent(OGREnvelope *psExtent, int bForce)

{
  return poDS->GetDSExtent(psExtent, bForce);
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/************************************************************************/
int OGRS57Layer::GetFeatureCount(int bForce) {
  if (poFilterGeom != NULL /*|| m_poAttrQuery != NULL*/
      || nFeatureCount == -1)
    return OGRLayer::GetFeatureCount(bForce);
  else
    return nFeatureCount;
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGRS57Layer::GetFeature(long nFeatureId)

{
  S57Reader *poReader = poDS->GetModule(0);  // not multi-reader aware

  if (poReader != NULL) {
    OGRFeature *poFeature;

    poFeature = poReader->ReadFeature(nFeatureId, poFeatureDefn);
    if (poFeature != NULL && poFeature->GetGeometryRef() != NULL)
      poFeature->GetGeometryRef()->assignSpatialReference(GetSpatialRef());
    return poFeature;
  } else
    return NULL;
}

/************************************************************************/
/*                           CreateFeature()                            */
/************************************************************************/

OGRErr OGRS57Layer::CreateFeature(OGRFeature *poFeature)

{
  /* -------------------------------------------------------------------- */
  /*      Set RCNM if not already set.                                    */
  /* -------------------------------------------------------------------- */
  int iRCNMFld = poFeature->GetFieldIndex("RCNM");

  if (iRCNMFld != -1) {
    if (!poFeature->IsFieldSet(iRCNMFld)) {
      poFeature->SetField(iRCNMFld, nRCNM);
    } else {
      CPLAssert(poFeature->GetFieldAsInteger(iRCNMFld) == nRCNM);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Set OBJL if not already set.                                    */
  /* -------------------------------------------------------------------- */
  if (nOBJL != -1) {
    int iOBJLFld = poFeature->GetFieldIndex("OBJL");

    if (!poFeature->IsFieldSet(iOBJLFld)) {
      poFeature->SetField(iOBJLFld, nOBJL);
    } else {
      CPLAssert(poFeature->GetFieldAsInteger(iOBJLFld) == nOBJL);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Create the isolated node feature.                               */
  /* -------------------------------------------------------------------- */
  //    if( poDS->GetWriter()->WriteCompleteFeature( poFeature ) )
  //        return OGRERR_NONE;
  //    else
  return OGRERR_FAILURE;
}
