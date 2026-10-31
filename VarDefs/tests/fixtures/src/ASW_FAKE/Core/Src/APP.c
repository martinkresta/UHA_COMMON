/* fake application for the ELM --src-root test */
void APP_Init(void)
{
	ELM_AddMeter(ELM_FOO, EL1_Pin, VAR_CONS_FOO_WH);      /* ok */
	ELM_AddMeter(ELM_BAT, EL2_Pin, VAR_BAT_SOC);          /* W: not a CONS_*_WH in el_meters */
	ELM_AddMeter(ELM_X, EL3_Pin, VAR_UNKNOWN_WH);         /* W: not in YAML */
//	ELM_AddMeter(ELM_Y, EL4_Pin, VAR_COMMENTED_WH);       commented out -> ignored
	/* ELM_AddMeter(ELM_Z, EL5_Pin, VAR_BLOCK_COMMENT_WH); */
}
