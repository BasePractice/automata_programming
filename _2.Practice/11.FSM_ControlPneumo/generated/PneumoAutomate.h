
/*  ------------------------------------------------------
     Routine name:  PneumoAutomate
     Description:   
     Project file:  automate.prt

------------------------------------------------------  */

/*  --- Base generator data types --- */
/* Real data type */
typedef double g_real_type;
/* Integer data type */
typedef int g_int_type;
/* Boolean data type */
typedef char g_boolean_type;
/* Complex data type */
typedef complex_64 g_complex_type;


/* Default initialization values */
const char input_0_default=0;
const char input_1_default=0;
const char input_2_default=0;
const char input_3_default=0;
const char out_0_default=0;
const char out_1_default=0;
const char out_2_default=1;
const double pneumoautomatev4_out_0_default=0;
const double pneumoautomatev4_out_1_default=0;
const double pneumoautomatev4_out_2_default=1;
const char pneumoautomatev0_out_0_default=0;
const char pneumoautomatev1_out_0_default=0;
const char pneumoautomatev2_out_0_default=0;
const char pneumoautomatev3_out_0_default=0;

/* Project signal database hash */
const unsigned int sp_database_hash_32=0;
/* Project sheme structure hash */
const unsigned int sp_sheme_hash_32=2293132410;


/* Structures defines */
/*  --- Source model preferences --- */
/* Minimum integration step */
#define INTEGRATION_MIN_STEP 0.001
/* Maximum integration step */
#define INTEGRATION_MAX_STEP 0.001
/* Integration synchronization step */
#define INTEGRATION_SYNC_STEP 0
/* Model integration method */
#define INTEGRATION_METHOD 0
/* Model relative error */
#define INTEGRATION_RELATIVE_ERROR 0.0001
/* Model absolute error */
#define INTEGRATION_ABSOLUTE_ERROR 1E-6
/* Model end time */
#define INTEGRATION_END_TIME 10
/* Model maximum iteration count */
#define INTEGRATION_MAX_LOOP_ITER_COUNT 10

/* External variables */
#define ext_vars_count 7

#define input_0 (*(char*)(ext_vars_addr[0]))
#define input_1 (*(char*)(ext_vars_addr[1]))
#define input_2 (*(char*)(ext_vars_addr[2]))
#define input_3 (*(char*)(ext_vars_addr[3]))
#define out_0 (*(char*)(ext_vars_addr[4]))
#define out_1 (*(char*)(ext_vars_addr[5]))
#define out_2 (*(char*)(ext_vars_addr[6]))

const ext_var_info_record ext_vars_names[ext_vars_count] = {
  {"input:0", "", (void*)&input_0_default, vt_bool,   {1}, 0, dir_input, sizeof(char)} ,
  {"input:1", "", (void*)&input_1_default, vt_bool,   {1}, 1, dir_input, sizeof(char)} ,
  {"input:2", "", (void*)&input_2_default, vt_bool,   {1}, 2, dir_input, sizeof(char)} ,
  {"input:3", "", (void*)&input_3_default, vt_bool,   {1}, 3, dir_input, sizeof(char)} ,
  {"out:0", "", (void*)&out_0_default, vt_bool,   {1}, 4, dir_out, sizeof(char)} ,
  {"out:1", "", (void*)&out_1_default, vt_bool,   {1}, 5, dir_out, sizeof(char)} ,
  {"out:2", "", (void*)&out_2_default, vt_bool,   {1}, 6, dir_out, sizeof(char)} 
};
/* State variables */
#define state_vars_count 7


typedef struct {
  double pneumoautomatev4_out_0_;
  double pneumoautomatev4_out_1_;
  double pneumoautomatev4_out_2_;
  char pneumoautomatev0_out_0_;
  char pneumoautomatev1_out_0_;
  char pneumoautomatev2_out_0_;
  char pneumoautomatev3_out_0_;
} t_state_vars;



const ext_var_info_record state_vars_names[state_vars_count] = {
  {"pneumoautomatev4_out_0", "Language out", (void*)&pneumoautomatev4_out_0_default, vt_double,   {1}, 0, dir_input, sizeof(double)} ,
  {"pneumoautomatev4_out_1", "Language out", (void*)&pneumoautomatev4_out_1_default, vt_double,   {1}, 8, dir_input, sizeof(double)} ,
  {"pneumoautomatev4_out_2", "Language out", (void*)&pneumoautomatev4_out_2_default, vt_double,   {1}, 16, dir_input, sizeof(double)} ,
  {"pneumoautomatev0_out_0", "Input pin state variable", (void*)&pneumoautomatev0_out_0_default, vt_bool,   {1}, 24, dir_input, sizeof(char)} ,
  {"pneumoautomatev1_out_0", "Input pin state variable", (void*)&pneumoautomatev1_out_0_default, vt_bool,   {1}, 25, dir_input, sizeof(char)} ,
  {"pneumoautomatev2_out_0", "Input pin state variable", (void*)&pneumoautomatev2_out_0_default, vt_bool,   {1}, 26, dir_input, sizeof(char)} ,
  {"pneumoautomatev3_out_0", "Input pin state variable", (void*)&pneumoautomatev3_out_0_default, vt_bool,   {1}, 27, dir_input, sizeof(char)} 
};
typedef char t_consts;
typedef char t_local;
#include "E:\GitHub\automata_programming\_2.Practice\11.FSM_ControlPneumo\pneumo_ctrl.h"
#include "E:\GitHub\automata_programming\_2.Practice\11.FSM_ControlPneumo\pneumo_ctrl.c"
struct PneumoEngine engine;
