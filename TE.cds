<distribution version="9.0.0" name="Oscilloscope" type="MSI">
	<prebuild>
		<workingdir>workspacedir</workingdir>
		<actions></actions></prebuild>
	<postbuild>
		<workingdir>workspacedir</workingdir>
		<actions></actions></postbuild>
	<msi GUID="{1BE46BA2-F0EC-48D5-A363-24910D171B1A}">
		<general appName="NIDAQmx Oscilloscope" outputLocation="d:\University\3rd\Procesamiento Digital de Señales\TE\cvidistkit.NIDAQmx Oscilloscope" relOutputLocation="cvidistkit.NIDAQmx Oscilloscope" outputLocationWithVars="d:\University\3rd\Procesamiento Digital de Señales\TE\cvidistkit.%name" relOutputLocationWithVars="cvidistkit.%name" autoIncrement="true" version="1.0.7">
			<arp company="Carlos Abraham Pérez Marrero" companyURL="" supportURL="" contact="carlos.perezm@estudiantes.uo.edu.cu" phone="+5354801672" comments=""/>
			<summary title="NIDAQmx Oscilloscope" subject="" keyWords="" comments="" author="Carlos Abraham Pérez Marrero"/></general>
		<userinterface language="English" readMe="" license="">
			<installerImages>
				<banner>
					<path>c:\Users\ABRAHAM\Downloads\OscilloscopeBanner.bmp</path></banner></installerImages>
			<dlgstrings welcomeTitle="NIDAQmx Oscilloscope Installer" welcomeText=""/></userinterface>
		<dirs appDirID="100">
			<installDir name="[Program Files]" dirID="2" parentID="-1" isMSIDir="true" visible="true"/>
			<installDir name="NIDAQmx Oscilloscope" dirID="100" parentID="2" isMSIDir="false" visible="true"/>
			<installDir name="[Start&gt;&gt;Programs]" dirID="7" parentID="-1" isMSIDir="true" visible="true"/>
			<installDir name="[Start Menu]" dirID="9" parentID="-1" isMSIDir="true" visible="true"/>
			<installDir name="Oscilloscope App 1.0" dirID="101" parentID="7" isMSIDir="false" visible="true"/></dirs>
		<files>
			<simpleFile fileID="0" sourcePath="d:\University\3rd\Procesamiento Digital de Señales\TE\Oscilloscope.exe" targetDir="100" readonly="false" hidden="false" system="false" regActiveX="false" runAfterInstallStyle="IMMEDIATELY_RESUME_INSTALL" cmdLineArgs="" runAfterInstall="false" uninstCmdLnArgs="" runUninst="false"/>
			<simpleFile fileID="1" sourcePath="C:\Program Files (x86)\National Instruments\Shared\CVI\toolslib\custctrl\daqmxioctrl.fp" relSourceBase="0" targetDir="100" readonly="false" hidden="false" system="false" regActiveX="false" runAfterInstallStyle="IMMEDIATELY_RESUME_INSTALL" cmdLineArgs="" runAfterInstall="false" uninstCmdLnArgs="" runUninst="false"/></files>
		<fileGroups>
			<projectOutput targetType="0" dirID="100" projectID="0">
				<fileID>0</fileID></projectOutput>
			<projectDependencies dirID="100" projectID="0"/></fileGroups>
		<shortcuts>
			<shortcut name="NIDAQmx Oscilloscope" targetFileID="0" destDirID="9" cmdLineArgs="" description="" runStyle="NORMAL"/></shortcuts>
		<mergemodules/>
		<products/>
		<runtimeEngine cvirte="true" instrsup="true" lvrt="true" analysis="true" netvarsup="false" dotnetsup="false" activeXsup="true" lowlevelsup="true" rtutilsup="true" installToAppDir="true"/>
		<advanced mediaSize="650">
			<launchConditions>
				<condition>MINOS_WINXP_SP2</condition>
			</launchConditions>
			<hardwareConfig></hardwareConfig>
			<includeConfigProducts>true</includeConfigProducts>
			<maxImportVisible>silent</maxImportVisible>
			<maxImportMode>merge</maxImportMode></advanced>
		<Projects NumProjects="1">
			<Project000 ProjectID="0" ProjectAbsolutePath="d:\University\3rd\Procesamiento Digital de Señales\TE\TE.prj" ProjectRelativePath="TE.prj"/></Projects>
		<buildData progressBarRate="0.086431094217866">
			<progressTimes>
					<Begin>0.000000000000000</Begin>
					<ProductsAdded>0.296999931335449</ProductsAdded>
					<DPConfigured>0.578000068664551</DPConfigured>
					<DPMergeModulesAdded>8.563000202178955</DPMergeModulesAdded>
					<DPClosed>9.610000133514404</DPClosed>
					<DistributionsCopied>9.625999927520752</DistributionsCopied>
					<End>11.569910216331483</End></progressTimes></buildData>
	</msi>
</distribution>
