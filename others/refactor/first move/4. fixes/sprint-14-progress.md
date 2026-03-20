# Sprint 14 Progress: Controller UI Decoupling

## Prerequisite: Sprint 13
Status: SATISFIED

Sprint 13 extracted pipeline into PipelineProcessor, reducing UserPanel complexity and making controller rewiring easier.

## Task 14.1: RFController — remove Ui::UserPanel*
Status: TODO

**Scope:** 1 m_ui-> access (DISTANCE->setText)

## Task 14.2: ScanController — remove Ui::UserPanel*
Status: TODO

**Scope:** 5 m_ui-> accesses (SCAN_BUTTON text/setText, CONTINUE_SCAN_BUTTON, RESTART_SCAN_BUTTON)

## Task 14.3: AuxPanelManager — remove Ui::UserPanel*
Status: TODO

**Scope:** 30 m_ui-> accesses (MISC_OPTION combos, MISC_RADIO buttons, MISC_DISPLAY stack)

## Task 14.4: LensController — remove Ui::UserPanel*
Status: TODO

**Scope:** 26 m_ui-> accesses (button text changes, edit text read/write, slider values)

## Task 14.5: LaserController — remove Ui::UserPanel* and Preferences*
Status: TODO

**Scope:** 23 m_ui-> accesses (button text/enable, checkbox), 5 m_pref-> accesses (LASER_ENABLE_CHK)

## Task 14.6: DeviceManager — remove Ui::UserPanel* and Preferences*
Status: TODO

**Scope:** 44 m_ui-> accesses (PTZ edits/sliders, COM edits, VID camera buttons), 13 m_pref-> accesses (ptz_type_enabled, display_baudrate, TCP_SERVER)

## Task 14.7: TCUController — remove Ui::UserPanel* and Preferences*
Status: TODO

**Scope:** 77 m_ui-> accesses (delay/gate-width/MCP/laser-width edits/sliders, layout changes), 9 m_pref-> accesses (PS stepping, auto rep freq, laser/MCP checkboxes, dist_ns)
