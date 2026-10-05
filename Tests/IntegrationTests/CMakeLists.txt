#===================== Integration tests configuration ========================#
# Integration tests of Tim module on target (Nucleo board). Registered in module
# CMakeLists.txt by IntegrationTesting_AddPath() and created by
# IntegrationTesting_Generate() in "IntegrationTest" build
# (see EmBi_Platform/CMake/IntegrationTesting/README.md).

IntegrationTesting_Add_Test(
    NAME            Tim
    TEST_SOURCE     ItTest_Tim.c
    LINK_LIBS       Tim_Lib             # incl. Gpio_Lib - pin level of timer output (TIM1_CH1)
    TIMEOUT         90
)
