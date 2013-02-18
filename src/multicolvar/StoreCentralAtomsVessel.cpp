/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2012 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed-code.org for more information.

   This file is part of plumed, version 2.0.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "vesselbase/VesselRegister.h"
#include "vesselbase/ActionWithVessel.h"
#include "MultiColvar.h"
#include "StoreCentralAtomsVessel.h"

namespace PLMD {
namespace multicolvar {

PLUMED_REGISTER_VESSEL(StoreCentralAtomsVessel,"CATOM_STASH")

void StoreCentralAtomsVessel::registerKeywords( Keywords& keys ){
  Vessel::registerKeywords( keys );
  plumed_assert( keys.size()==0 );
}

void StoreCentralAtomsVessel::reserveKeyword( Keywords& keys ){
  keys.reserve("optional","CATOM_STASH","This documentation shouldn't appear");
}

StoreCentralAtomsVessel::StoreCentralAtomsVessel( const vesselbase::VesselOptions& da ):
Vessel(da),
wasforced(false)
{
  mycolv=dynamic_cast<MultiColvar*>( getAction() );
  plumed_assert( mycolv );
}

void StoreCentralAtomsVessel::resize(){
  unsigned nfunc=mycolv->getNumberOfFunctionsInAction();
  unsigned bsize=0; start.resize( nfunc +1 );
  for(unsigned i=0;i<nfunc;++i){
      start[i] = bsize;
      bsize += 3*( 1 + mycolv->getNumberOfDerivatives(i) );
  }
  start[nfunc]=bsize;
  resizeBuffer( bsize ); 
  forces.resize( mycolv->getNumberOfDerivatives() );
}

void StoreCentralAtomsVessel::prepare(){
  wasforced=false;
  forces.assign( forces.size(), 0.0 );
}

bool StoreCentralAtomsVessel::calculate(){
  Vector catom_pos=mycolv->retrieveCentralAtomPos( false );
  Vector wdf; wdf.zero(); 

  unsigned ibuf=start[mycolv->current];
  for(unsigned i=0;i<3;++i){
      addToBufferElement( ibuf, catom_pos[i] ); ibuf++; wdf[i]=1.0;
      for(unsigned j=0;j<mycolv->getNAtoms();++j){
         addToBufferElement( ibuf, mycolv->getCentralAtomDerivative( j, i, wdf ) ); ibuf++;
      }
      wdf.zero();
  }
  plumed_dbg_assert( ibuf==start[mycolv->current+1] );
  return true;
}

Vector StoreCentralAtomsVessel::getPosition( const unsigned& ivec ) const {
  plumed_dbg_assert( ivec<mycolv->getNumberOfFunctionsInAction() );
  unsigned pos=start[ivec]; Vector mypos;
  for(unsigned i=0;i<3;++i){
      mypos[i] = getBufferElement( pos ); pos+=mycolv->getNumberOfDerivatives(ivec)+1;
  }
  plumed_dbg_assert( pos==start[ivec+1] );
  return mypos;
}

void StoreCentralAtomsVessel::addForces( const std::vector<double>& ff){
  plumed_dbg_assert( ff.size()==forces.size() );
  wasforced=true;
  for(unsigned i=0;i<forces.size();++i) forces[i]+=ff[i];
}

bool StoreCentralAtomsVessel::applyForce(std::vector<double>& ff){
  plumed_dbg_assert( ff.size()==forces.size() );
  if(wasforced){
    for(unsigned i=0;i<forces.size();++i) ff[i]=forces[i];
  }
  return wasforced;
}

void StoreCentralAtomsVessel::chainRuleForCentralAtom( const unsigned& iatom, const unsigned& iderno, const Vector& df, vesselbase::ActionWithVessel* act) const {
  plumed_dbg_assert( iatom<mycolv->getNumberOfFunctionsInAction() );

  unsigned nder=mycolv->getNumberOfDerivatives(iatom);
  unsigned nder2=mycolv->getNumberOfDerivatives();
  for(unsigned ider=0;ider<nder;++ider){
      for(unsigned jcomp=0;jcomp<3;++jcomp){
          unsigned ibuf=start[iatom] + jcomp*(nder+1) + ider;
          act->addElementDerivative( iderno*nder2 + mycolv->getOutputDerivativeIndex(iatom, ider), df[jcomp]*getBufferElement(ibuf) );
      }
  }
}
 

}
}
