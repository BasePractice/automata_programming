/*  ------------------------------------------------------
     Routine name:  PneumoAutomate
     Description:   
     Project file:  automate.prt

------------------------------------------------------  */

int i;
int j;
int c;
ret = 0;
memcpy(&input_0,&input_0_default,sizeof(input_0_default));
memcpy(&input_1,&input_1_default,sizeof(input_1_default));
memcpy(&input_2,&input_2_default,sizeof(input_2_default));
memcpy(&input_3,&input_3_default,sizeof(input_3_default));
memcpy(&out_0,&out_0_default,sizeof(out_0_default));
memcpy(&out_1,&out_1_default,sizeof(out_1_default));
memcpy(&out_2,&out_2_default,sizeof(out_2_default));
memcpy(&state_vars->pneumoautomatev4_out_0_,&pneumoautomatev4_out_0_default,sizeof(pneumoautomatev4_out_0_default));
memcpy(&state_vars->pneumoautomatev4_out_1_,&pneumoautomatev4_out_1_default,sizeof(pneumoautomatev4_out_1_default));
memcpy(&state_vars->pneumoautomatev4_out_2_,&pneumoautomatev4_out_2_default,sizeof(pneumoautomatev4_out_2_default));
memcpy(&state_vars->pneumoautomatev0_out_0_,&pneumoautomatev0_out_0_default,sizeof(pneumoautomatev0_out_0_default));
memcpy(&state_vars->pneumoautomatev1_out_0_,&pneumoautomatev1_out_0_default,sizeof(pneumoautomatev1_out_0_default));
memcpy(&state_vars->pneumoautomatev2_out_0_,&pneumoautomatev2_out_0_default,sizeof(pneumoautomatev2_out_0_default));
memcpy(&state_vars->pneumoautomatev3_out_0_,&pneumoautomatev3_out_0_default,sizeof(pneumoautomatev3_out_0_default));
pneumo_engine_init(&engine);
