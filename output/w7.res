        ��  ��                  d
      �� ��     0 	        <?xml version="1.0" encoding="utf-8" standalone="yes"?>
<asmv1:assembly manifestVersion="1.0" xmlns="urn:schemas-microsoft-com:asm.v1" 
                xmlns:asmv1="urn:schemas-microsoft-com:asm.v1" 
                xmlns:asmv2="urn:schemas-microsoft-com:asm.v2" 
                xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">

  <assemblyIdentity 
	version="1.0.0.0" 
	type="win32" 
	processorArchitecture="*" 
	name="textedit.exe"
  />

    <!-- Disable file and registry virtualization. -->
    <trustInfo xmlns="urn:schemas-microsoft-com:asm.v2">
      <security>
        <requestedPrivileges xmlns="urn:schemas-microsoft-com:asm.v3">
         <requestedExecutionLevel  level="asInvoker" uiAccess="false" />
         <!--  <requestedExecutionLevel  level="asInvoker" uiAccess="false" />
               <requestedExecutionLevel  level="requireAdministrator" uiAccess="false" />
               <requestedExecutionLevel  level="highestAvailable" uiAccess="false" />
         -->
        </requestedPrivileges>
      </security>
    </trustInfo>

    <!-- We are high-dpi aware on Windows Vista -->
    <asmv3:application xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">
      <asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">
        <dpiAware>true</dpiAware>
      </asmv3:windowsSettings>
    </asmv3:application>

    <!-- Declare that we were designed to work with Windows Vista and Windows 7-->
    <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
      <application>
        <!--The ID below indicates application support for Windows 10 -->
        <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/>
        <!--The ID below indicates application support for Windows 8.1 -->
        <supportedOS Id="{1f676c76-80e1-4239-95bb-83d0f6d0da78}"/>		
        <!--The ID below indicates application support for Windows Vista -->
        <supportedOS Id="{e2011457-1546-43c5-a5fe-008deee3d3f0}"/>
        <!--The ID below indicates application support for Windows 7 -->
        <supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}"/>
      </application>
    </compatibility>

    <!-- Enable themes for Windows common controls and dialogs (Windows XP and later) -->
    <dependency>
      <dependentAssembly>
        <assemblyIdentity
            type="win32"
            name="Microsoft.Windows.Common-Controls"
            version="6.0.0.0"
            processorArchitecture="*"
            publicKeyToken="6595b64144ccf1df"
            language="*"
        />
      </dependentAssembly>
    </dependency>

  </asmv1:assembly>