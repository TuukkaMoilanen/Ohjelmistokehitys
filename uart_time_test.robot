*** Settings ***
Library    OperatingSystem
Library    Process
Library    Collections

*** Variables ***
${PORT}        COM5
${BAUD}        115200

*** Keywords ***
Send Command And Get Response
    [Arguments]    ${cmd}
    ${args}=    Create List    -c    import serial,sys; s=serial.Serial('${PORT}',${BAUD},timeout=2); s.write(b"${cmd}\\r"); print(s.readline().decode().strip()); s.close()
    ${process}=    Start Process    python    @{args}    shell=False    stdout=PIPE    stderr=PIPE
    ${result}=    Wait For Process    ${process}    timeout=5s
    ${output}=    Set Variable    ${result.stdout}
    Log    Got: ${output}
    RETURN    ${output}

*** Test Cases ***
Valid Time -> 80 seconds
    ${res}=    Send Command And Get Response    000120
    Should Contain    ${res}    RES:80

Invalid Time -> error code -1
    ${res}=    Send Command And Get Response    001067
    Should Contain    ${res}    RES:-1
