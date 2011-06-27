#include "Hom_Basic.hh"


typedef enum comparator { EQ, NEQ, LT, GT, LEQ, GEQ} comparator;

std::string to_string (comparator comp) {
  switch (comp) {
    case EQ : 
      return "==";
    case NEQ :
      return "!=";
    case LT :
      return "<";
    case GT :
      return ">";
    case LEQ :
      return "<=";
    case GEQ :
      return ">=";
  default :
    return "??";
    }
}

class _VarCompState:public StrongHom {

  int var;
  int val;
  comparator comp;
public:
  _VarCompState(int vr, comparator c, int vl) : var(vr), val(vl), comp(c) {}
  
  bool
  skip_variable(int vr) const
  {
    return vr != var;
  }
  
  GDDD phiOne() const {
    return GDDD::one;
  }                   
  
  GHom phi(int vr, int vl) const {
    bool sel = false;
    switch (comp) {
    case EQ : 
      sel = (vl == val);
      break;
    case NEQ :
      sel = (vl != val);
      break ;
    case LT :
      sel = (vl < val);
      break;
    case GT :
      sel = (vl > val);
      break ;
    case LEQ :
      sel = (vl <= val);
      break;
    case GEQ :
      sel = (vl >= val);
      break;
    }
    if (sel)
      return GHom(vr, vl, GHom::id);
    else
      return GHom(GDDD::null);
  }
  
  size_t hash() const {
    return 8097*(var+2)^val * comp;
  }
  
  bool is_selector () const {
    return true;
  }

  // invert : already handled by default selector implem

  void print (std::ostream & os) const {
    os << "[ " << DDD::getvarName(var) << " " << to_string(comp) << " " << val << " ]";
  }

  bool operator==(const StrongHom &s) const {
    _VarCompState* ps = (_VarCompState*)&s;
    return comp == ps->comp && var == ps->var && val == ps->val;
  }
    _GHom * clone () const {  return new _VarCompState(*this); }
};

GHom varCompState (int var, comparator c , int val) {
  return _VarCompState(var, c , val);
}

GHom varEqState (int var, int val) {
  return varCompState(var,EQ,val);
}

GHom varNeqState (int var, int val) {
  return varCompState (var,NEQ,val);
}

GHom varGtState (int var, int val) {
  return varCompState (var,GT,val);
}

GHom varLeqState (int var, int val) {
 return varCompState (var,LEQ,val);
}

GHom varLtState (int var, int val) {
  return varCompState (var,LT,val);
}

GHom varGeqState (int var, int val) {
 return varCompState (var,GEQ,val);
}




class _setVarConst:public StrongHom {

  int var;
  int val;
public:
  _setVarConst(int vr, int vl) : var(vr), val(vl) {}
  
  bool
  skip_variable(int vr) const
  {
    return vr != var;
  }
  
  GDDD phiOne() const {
    return GDDD::one;
  }                   
  
  GHom phi(int vr, int vl) const {
    if (var == vr)
      return GHom(vr, val, GHom::id);
    return GHom(vr, vl, GHom(this)); 
  }

  GHom invert (const GDDD & potall) const { 
    std::set<GHom> sum;
    GDDD pot = computeDomain(var,potall);
    for (GDDD::const_iterator it = pot.begin() ; it != pot.end() ; ++it ) {
      sum.insert ( setVarConst(var,it->first) );
    }
    return GHom::add(sum) & varEqState (var,val);
  }

  
  size_t hash() const {
    return 6619*(var+13)^val;
  }
  void print (std::ostream & os) const {
    os << "[ " << DDD::getvarName(var) << " = "  << val << " ]";
  }

  bool operator==(const StrongHom &s) const {
    _setVarConst* ps = (_setVarConst*)&s;
    return var == ps->var && val == ps->val;
  }
    _GHom * clone () const {  return new _setVarConst(*this); }
};

GHom setVarConst (int var, int val) {
  return _setVarConst(var,val);
}

class _incVar :public StrongHom {
  int target;
  int val;
public:
  _incVar (int var, int val) : target(var), val(val) {}
  
  bool
  skip_variable(int var) const
  {
    return target != var;
  }
    
  GHom invert (const GDDD & pot) const { 
    return incVar(target, -val);
   }


  GDDD phiOne() const {
    return GDDD::one;
  }                   

  GHom phi(int vr, int vl) const {
    if (target == vr) {
      // Just a trick to detect if we are going unbounded ...
      //	    if (vl > 1) std::cerr << "reached value " << vl << " for place " << DDD::getvarName(vr) << std::endl; 
      return GHom(vr, vl + val );
    } else {
      // Should not happen with skip_variable
      return GHom(vr,vl,this);
    }
  }
  
  size_t hash() const {
    return 9049* target^val;
  }
  
  
  bool operator==(const StrongHom &s) const {
    _incVar* ps = (_incVar*)&s;
    return target == ps->target && val == ps->val;
  }

  _GHom * clone () const {  return new _incVar(*this); }

  void print (std::ostream & os) const {
    os << "[ " << DDD::getvarName(target) << " += " << val << " ]";
  }

};



GHom incVar (int var, int val) {
  return _incVar(var,val);
}

static comparator invertComp(comparator comp) {
	switch (comp) {
    case EQ : case NEQ :
      return comp;
    case LT :
      return GT;
    case GT :
      return LT;
    case LEQ :
      return GEQ;
    case GEQ :
      return LEQ;
    default:
      assert(false);
      return EQ;
    }
}

class _VarCompVar : public StrongHom {	
	int var1;
	int var2;
	comparator c;
public:
	_VarCompVar(int v1, comparator c, int v2) : var1(v1), var2(v2), c(c) {}
	
	bool skip_variable(int vr) const {
		return vr != var1 && vr != var2;
	}
	
	GDDD phiOne() const {
		return GDDD::one;
	}                   
	
	GHom phi(int vr, int vl) const {
		if (vr == var1) {
			return GHom(vr,vl,varCompState(var2,c,vl));
		} else { // vr == var2
			return GHom(vr,vl,varCompState(var1,invertComp(c),vl));
		}
	}
	
	size_t hash() const {
		return 6073*(var1+2)^var2 * c;
	}
	
	bool is_selector () const {
		return true;
	}
	
	// invert : already handled by default selector implem
	
	void print (std::ostream & os) const {
		os << "[ " << DDD::getvarName(var1) << to_string(c) << DDD::getvarName(var2) << " ]";
	}
	
	bool operator==(const StrongHom &s) const {
		_VarCompVar* ps = (_VarCompVar*)&s;
		return var1 == ps->var1 && var2 == ps->var2 && c == ps->c;
	}
    _GHom * clone () const {  return new _VarCompVar(*this); }
};

GHom varCompVar (int var, comparator c , int var2) {
  return _VarCompVar(var, c , var2);
}

GHom varEqVar (int var, int var2) {
  return varCompVar(var,EQ,var2);
}

GHom varNeqVar (int var, int var2) {
  return varCompVar (var,NEQ,var2);
}

GHom varGtVar (int var, int var2) {
  return varCompVar (var,GT,var2);
}

GHom varLeqVar (int var, int var2) {
 return varCompVar (var,LEQ,var2);
}

GHom varLtVar (int var, int var2) {
  return varCompVar (var,LT,var2);
}

GHom varGeqVar (int var, int var2) {
 return varCompVar (var,GEQ,var2);
}
