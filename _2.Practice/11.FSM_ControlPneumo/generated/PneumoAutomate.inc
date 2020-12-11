/*  ------------------------------------------------------
     Routine name:  PneumoAutomate
     Description:   
     Project file:  automate.prt

------------------------------------------------------  */

/*       Local stack variables                */
int i;
int j;
int c;
int itmp1;
int itmp2;
int itmp3;
double tmp1;
double tmp2;
double tmp3;
double tmp4;
double tmp5;
double tmp6;
double tmp7;
char f;
char tmp_f_1;
char u_s;
char u_r;
ret = 0;

switch (action){
case f_Stop:{
};break;
case f_GetDeri:{
};break;
case f_GetAlgFun:{
};break;
default:{

/* Index=0
   UID=0
   GeneratorClassName=TInputPin
   Name=Const_source18
   Type=Входной контакт */

state_vars->pneumoautomatev0_out_0_ = ((input_0));

/* Index=1
   UID=1
   GeneratorClassName=TInputPin
   Name=Const_source19
   Type=Входной контакт */

state_vars->pneumoautomatev1_out_0_ = ((input_1));

/* Index=2
   UID=2
   GeneratorClassName=TInputPin
   Name=Const_source20
   Type=Входной контакт */

state_vars->pneumoautomatev2_out_0_ = ((input_2));

/* Index=3
   UID=3
   GeneratorClassName=TInputPin
   Name=Const_source21
   Type=Входной контакт */

state_vars->pneumoautomatev3_out_0_ = ((input_3));

/* Index=4
   UID=4
   GeneratorClassName=TLanguage
   Name=Automate
   Type=Язык программирования */

engine.cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_UP] = state_vars->pneumoautomatev0_out_0_;
engine.cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN] = state_vars->pneumoautomatev1_out_0_;
engine.cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_UP] = state_vars->pneumoautomatev2_out_0_;
engine.cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN] = state_vars->pneumoautomatev3_out_0_;

state_vars->pneumoautomatev4_out_2_ = pneumo_engine_tick(&engine);
state_vars->pneumoautomatev4_out_0_ = engine.cylinders[PNEUMO_CYLINDER_Y1].output_signal;
state_vars->pneumoautomatev4_out_1_ = engine.cylinders[PNEUMO_CYLINDER_Y2].output_signal;

/* Index=5
   UID=5
   GeneratorClassName=TOutPin
   Name=OutPin15
   Type=Выходной контакт */

out_0 = (state_vars->pneumoautomatev4_out_0_ > 0.5);

/* Index=6
   UID=6
   GeneratorClassName=TOutPin
   Name=OutPin16
   Type=Выходной контакт */

out_1 = (state_vars->pneumoautomatev4_out_1_ > 0.5);

/* Index=7
   UID=7
   GeneratorClassName=TOutPin
   Name=OutPin17
   Type=Выходной контакт */

out_2 = (state_vars->pneumoautomatev4_out_2_ > 0.5);
};break;
};
