// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Scope.h"
#include "HeaderProvider.h"
#include "UnrealTypeDefinitionInfo.h"
#include "GeneratedCodeVersion.h"

class FClassMetaData;

/**
 * Contains information about source file that defines various UHT aware types.
 */
class FUnrealSourceFile
{
public:
	// Constructor.
	FUnrealSourceFile(UPackage* InPackage, FString InFilename, FString InContent)
		: Scope                (MakeShareable(new FFileScope(*(FString(TEXT("__")) + FPaths::GetBaseFilename(InFilename) + FString(TEXT("__File"))), this)))
		, Filename             (MoveTemp(InFilename))
		, Package              (InPackage)
		, bHasChanged          (false)
		, Content              (MoveTemp(InContent))
		, bParsed              (false)
		, bDependenciesResolved(false)
	{
		if (GetStrippedFilename() != "Object")
		{
			Includes.Add(FHeaderProvider(EHeaderProviderSourceType::FileName, "Object.h"));
		}
	}

	/**
	 * Appends array with classes defined in this source file.
	 *
	 * @param OutClasses (Output parameter) Array to fill with classes defined.
	 */
	void AppendDefinedClasses(TArray<UClass*>& OutClasses) const
	{
		for (auto* Class : DefinedClasses)
		{
			OutClasses.Add(Class);
		}
	}

	/**
	 * Gets array with classes defined in this source file.
	 *
	 * @returns Array with classes defined in this source file.
	 */
	const TArray<UClass*>& GetDefinedClasses() const
	{
		return DefinedClasses;
	}

	/**
	 * Gets number of types defined in this source file.
	 */
	int32 GetDefinedClassesCount() const
	{
		return GetDefinedClasses().Num();
	}

	/**
	 * Gets generated header filename.
	 */
	FString GetGeneratedHeaderFilename() const
	{
		return FString::Printf(TEXT("%s.generated.h"), *FPaths::GetBaseFilename(Filename));
	}

	/**
	 * Gets module relative path.
	 */
	const FString& GetModuleRelativePath() const
	{
		return ModuleRelativePath;
	}

	/**
	 * Gets stripped filename.
	 */
	FString GetStrippedFilename() const;

	/**
	 * Gets unique file id.
	 */
	FString GetFileId() const;

	/**
	 * Gets source file API.
	 */
	FString GetAPI() const;

	/**
	 * Gets define name of this source file.
	 */
	FString GetFileDefineName() const;

	/**
	 * Gets file-wise generated body macro name.
	 *
	 * @param LineNumber Number at which generated body macro is.
	 * @param bLegacy Tells if method should get legacy generated body macro.
	 */
	FString GetGeneratedBodyMacroName(int32 LineNumber, bool bLegacy = false) const;

	/**
	 * Gets file-wise generated body macro name.
	 *
	 * @param LineNumber Number at which generated body macro is.
	 * @param Suffix Suffix to add to generated body macro name.
	 */
	FString GetGeneratedMacroName(int32 LineNumber, const TCHAR* Suffix) const;

	/**
	 * Gets file-wise generated body macro name.
	 *
	 * @param ClassData Class metadata for which to get generated body macro name.
	 * @param Suffix Suffix to add to generated body macro name.
	 */
	FString GetGeneratedMacroName(FClassMetaData* ClassData, const TCHAR* Suffix = nullptr) const;

	/**
	 * Adds given class to class definition list for this source file.
	 *
	 * @param Class Class to add to list.
	 */
	void AddDefinedClass(UClass* Class);

	/**
	 * Gets scope for this file.
	 */
	TSharedRef<FFileScope> GetScope() const
	{
		return Scope;
	}

	/**
	 * Gets package this file is in.
	 */
	UPackage* GetPackage() const
	{
		return Package;
	}

	/**
	 * Gets filename.
	 */
	const FString& GetFilename() const
	{
		return Filename;
	}

	/**
	 * Gets generated filename.
	 */
	const FString& GetGeneratedFilename() const
	{
		return GeneratedFilename;
	}

	/**
	 * Gets include path.
	 */
	const FString& GetIncludePath() const
	{
		return IncludePath;
	}

	/**
	 * Gets content.
	 */
	const FString& GetContent() const;

	/**
	 * Gets includes.
	 */
	TArray<FHeaderProvider>& GetIncludes()
	{
		return Includes;
	}

	/**
	 * Gets includes. Const version.
	 */
	const TArray<FHeaderProvider>& GetIncludes() const
	{
		return Includes;
	}

	/**
	 * Gets generated code version for given UStruct.
	 */
	EGeneratedCodeVersion GetGeneratedCodeVersionForStruct(UStruct* Struct);

	/**
	 * Gets generated code versions.
	 */
	TMap<UStruct*, EGeneratedCodeVersion>& GetGeneratedCodeVersions()
	{
		return GeneratedCodeVersions;
	}

	/**
	 * Gets generated code versions. Const version.
	 */
	const TMap<UStruct*, EGeneratedCodeVersion>& GetGeneratedCodeVersions() const
	{
		return GeneratedCodeVersions;
	}

	/**
	 * Sets generated filename.
	 */
	void SetGeneratedFilename(FString GeneratedFilename);

	/**
	 * Sets has changed flag.
	 */
	void SetHasChanged(bool bHasChanged);

	/**
	 * Sets module relative path.
	 */
	void SetModuleRelativePath(FString ModuleRelativePath);

	/**
	 * Sets include path.
	 */
	void SetIncludePath(FString IncludePath);

	/**
	 * Mark this file as parsed.
	 */
	void MarkAsParsed();

	/**
	 * Checks if this file is parsed.
	 */
	bool IsParsed() const;

	/**
	 * Checks if generated file has been changed.
	 */
	bool HasChanged() const;

	/**
	 * Mark that this file has resolved dependencies.
	 */
	void MarkDependenciesResolved();

	/**
	 * Checks if dependencies has been resolved.
	 */
	bool AreDependenciesResolved() const;

private:
	// File scope.
	TSharedRef<FFileScope> Scope;

	// Path of this file.
	FString Filename;

	// Package of this file.
	UPackage* Package;

	// File name of the generated header file associated with this file.
	FString GeneratedFilename;

	// Tells if generated header file was changed.
	bool bHasChanged;

	// Module relative path.
	FString ModuleRelativePath;

	// Include path.
	FString IncludePath;

	// Source file content.
	FString Content;

	// Tells if this file was parsed.
	bool bParsed;

	// Tells if dependencies has been resolved already.
	bool bDependenciesResolved;

	// This source file includes.
	TArray<FHeaderProvider> Includes;

	// List of classes defined in this source file.
	TArray<UClass*> DefinedClasses;

	// Mapping of UStructs to versions, according to which their code should be generated.
	TMap<UStruct*, EGeneratedCodeVersion> GeneratedCodeVersions;
};