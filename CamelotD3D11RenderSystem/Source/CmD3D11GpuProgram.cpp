#include "CmD3D11GpuProgram.h"
#include "CmD3D11Device.h"
#include "CmException.h"
#include "CmDebug.h"
#include "CmGpuParams.h"
#include "CmD3D11RenderSystem.h"
#include "CmGpuProgramManager.h"
#include "CmHardwareBufferManager.h"
#include "CmD3D11HLSLParamParser.h"
#include "CmD3D11GpuProgramRTTI.h"

namespace BansheeEngine
{
	UINT32 D3D11GpuProgram::GlobalProgramId = 0;

	D3D11GpuProgram::D3D11GpuProgram(const String& source, const String& entryPoint, GpuProgramType gptype, 
		GpuProgramProfile profile, const Vector<HGpuProgInclude>* includes, bool isAdjacencyInfoRequired)
		: GpuProgram(source, entryPoint, gptype, profile, includes, isAdjacencyInfoRequired),
		mColumnMajorMatrices(true), mEnableBackwardsCompatibility(false), mProgramId(0)
	{

	}

	D3D11GpuProgram::~D3D11GpuProgram()
	{

	}

	void D3D11GpuProgram::initialize_internal()
	{
		if (!isSupported())
		{
			mIsCompiled = false;
			mCompileError = "Specified program is not supported by the current render system.";

			return;
		}

		D3D11RenderSystem* rs = static_cast<D3D11RenderSystem*>(RenderSystem::instancePtr());
		String hlslProfile = rs->getCapabilities()->gpuProgProfileToRSSpecificProfile(mProfile);

		ID3DBlob* microcode = compileMicrocode(hlslProfile);

		if (microcode != nullptr)
		{
			mMicrocode.resize(microcode->GetBufferSize());
			memcpy(&mMicrocode[0], microcode->GetBufferPointer(), microcode->GetBufferSize());

			populateParametersAndConstants(microcode);
			loadFromMicrocode(rs->getPrimaryDevice(), microcode);

			SAFE_RELEASE(microcode);
		}

		mProgramId = GlobalProgramId++;

		GpuProgram::initialize_internal();
	}

	void D3D11GpuProgram::destroy_internal()
	{
		mMicrocode.clear();

		mInputDeclaration = nullptr;

		GpuProgram::destroy_internal();
	}

	ID3DBlob* D3D11GpuProgram::compileMicrocode(const String& profile)
	{
		// TODO - Preprocessor defines aren't supported

		UINT compileFlags = 0;
#if defined(CM_DEBUG_MODE)
		compileFlags |= D3DCOMPILE_DEBUG;
		compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		if (mColumnMajorMatrices)
			compileFlags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
		else
			compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

		if (mEnableBackwardsCompatibility)
			compileFlags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;

		ID3DBlob* microCode = nullptr;
		ID3DBlob* errors = nullptr;

		HRESULT hr = D3DCompile(
			mSource.c_str(),	// [in] Pointer to the shader in memory. 
			mSource.size(),		// [in] Size of the shader in memory.  
			nullptr,			// [in] The name of the file that contains the shader code. 
			nullptr,			// [in] Optional. Pointer to a NULL-terminated array of macro definitions. See D3D_SHADER_MACRO. If not used, set this to NULL. 
			nullptr,			// [in] Optional. Pointer to an ID3DInclude Interface interface for handling include files. Setting this to NULL will cause a compile error if a shader contains a #include. 
			mEntryPoint.c_str(),// [in] Name of the shader-entrypoint function where shader execution begins. 
			profile.c_str(),// [in] A string that specifies the shader model; can be any profile in shader model 4 or higher. 
			compileFlags,		// [in] Effect compile flags - no D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY at the first try...
			0,					// [in] Effect compile flags
			&microCode,			// [out] A pointer to an ID3DBlob Interface which contains the compiled shader, as well as any embedded debug and symbol-table information. 
			&errors				// [out] A pointer to an ID3DBlob Interface which contains a listing of errors and warnings that occurred during compilation. These errors and warnings are identical to the the debug output from a debugger.
			);

		if (FAILED(hr))
		{
			mIsCompiled = false;
			mCompileError = "Cannot compile D3D11 high-level shader. Errors:\n" +
				String(static_cast<const char*>(errors->GetBufferPointer()));

			SAFE_RELEASE(microCode);
			SAFE_RELEASE(errors);
			return nullptr;
		}
		else
		{
			mIsCompiled = true;
			mCompileError = "";

			SAFE_RELEASE(errors);
			return microCode;
		}
	}

	void D3D11GpuProgram::populateParametersAndConstants(ID3DBlob* microcode)
	{
		assert(microcode != nullptr);

		D3D11HLSLParamParser parser;

		if (mType == GPT_VERTEX_PROGRAM)
			mInputDeclaration = HardwareBufferManager::instance().createVertexDeclaration();

		parser.parse(microcode, mParametersDesc, mInputDeclaration);
	}

	GpuParamsPtr D3D11GpuProgram::createParameters()
	{
		GpuParamsPtr params = cm_shared_ptr<GpuParams, PoolAlloc>(std::ref(mParametersDesc), mColumnMajorMatrices);
		return params;
	}

	const String& D3D11GpuProgram::getLanguage() const
	{
		static String name = "hlsl";

		return name;
	}

	/************************************************************************/
	/* 								SERIALIZATION                      		*/
	/************************************************************************/
	RTTITypeBase* D3D11GpuProgram::getRTTIStatic()
	{
		return D3D11GpuProgramRTTI::instance();
	}

	RTTITypeBase* D3D11GpuProgram::getRTTI() const
	{
		return D3D11GpuProgram::getRTTIStatic();
	}

	D3D11GpuVertexProgram::D3D11GpuVertexProgram(const String& source, const String& entryPoint,
		GpuProgramProfile profile, const Vector<HGpuProgInclude>* includes, bool isAdjacencyInfoRequired)
		: D3D11GpuProgram(source, entryPoint, GPT_VERTEX_PROGRAM, profile, includes, isAdjacencyInfoRequired)
		, mVertexShader(nullptr)
	{ }

	D3D11GpuVertexProgram::~D3D11GpuVertexProgram()
	{ }

	void D3D11GpuVertexProgram::loadFromMicrocode(D3D11Device& device, ID3D10Blob*  microcode)
	{
		HRESULT hr = device.getD3D11Device()->CreateVertexShader( 
			static_cast<DWORD*>(microcode->GetBufferPointer()), microcode->GetBufferSize(),
			device.getClassLinkage(), &mVertexShader);

		if (FAILED(hr) || device.hasError())
		{
			String errorDescription = device.getErrorDescription();
			CM_EXCEPT(RenderingAPIException, 
				"Cannot create D3D11 vertex shader from microcode\nError Description:" + errorDescription);

		}
	}

	void D3D11GpuVertexProgram::destroy_internal()
	{
		SAFE_RELEASE(mVertexShader);

		D3D11GpuProgram::destroy_internal();
	}

	ID3D11VertexShader * D3D11GpuVertexProgram::getVertexShader() const
	{
		return mVertexShader;
	}

	D3D11GpuFragmentProgram::D3D11GpuFragmentProgram(const String& source, const String& entryPoint,
		GpuProgramProfile profile, const Vector<HGpuProgInclude>* includes, bool isAdjacencyInfoRequired)
		: D3D11GpuProgram(source, entryPoint, GPT_FRAGMENT_PROGRAM, profile, includes, isAdjacencyInfoRequired)
		, mPixelShader(nullptr)
	{ }

	D3D11GpuFragmentProgram::~D3D11GpuFragmentProgram()
	{ }

	void D3D11GpuFragmentProgram::loadFromMicrocode(D3D11Device& device, ID3D10Blob* microcode)
	{
		HRESULT hr = device.getD3D11Device()->CreatePixelShader(
			static_cast<DWORD*>(microcode->GetBufferPointer()), microcode->GetBufferSize(),
			device.getClassLinkage(), &mPixelShader);

		if (FAILED(hr) || device.hasError())
		{
			String errorDescription = device.getErrorDescription();
			CM_EXCEPT(RenderingAPIException, 
				"Cannot create D3D11 pixel shader from microcode.\nError Description:" + errorDescription);
		}
	}

	void D3D11GpuFragmentProgram::destroy_internal()
	{
		SAFE_RELEASE(mPixelShader);

		D3D11GpuProgram::destroy_internal();
	}

	ID3D11PixelShader * D3D11GpuFragmentProgram::getPixelShader() const
	{
		return mPixelShader;
	}

	D3D11GpuGeometryProgram::D3D11GpuGeometryProgram(const String& source, const String& entryPoint,
		GpuProgramProfile profile, const Vector<HGpuProgInclude>* includes, bool isAdjacencyInfoRequired)
		: D3D11GpuProgram(source, entryPoint, GPT_GEOMETRY_PROGRAM, profile, includes, isAdjacencyInfoRequired)
		, mGeometryShader(nullptr)
	{ }

	D3D11GpuGeometryProgram::~D3D11GpuGeometryProgram()
	{ }

	void D3D11GpuGeometryProgram::loadFromMicrocode(D3D11Device& device, ID3D10Blob* microcode)
	{
		HRESULT hr = device.getD3D11Device()->CreateGeometryShader(
			static_cast<DWORD*>(microcode->GetBufferPointer()), microcode->GetBufferSize(),
			device.getClassLinkage(), &mGeometryShader);

		if (FAILED(hr) || device.hasError())
		{
			String errorDescription = device.getErrorDescription();
			CM_EXCEPT(RenderingAPIException, 
				"Cannot create D3D11 geometry shader from microcode.\nError Description:" + errorDescription);
		}
	}

	void D3D11GpuGeometryProgram::destroy_internal()
	{
		SAFE_RELEASE(mGeometryShader);

		D3D11GpuProgram::destroy_internal();
	}

	ID3D11GeometryShader * D3D11GpuGeometryProgram::getGeometryShader() const
	{
		return mGeometryShader;
	}

	D3D11GpuDomainProgram::D3D11GpuDomainProgram(const String& source, const String& entryPoint,
		GpuProgramProfile profile, const Vector<HGpuProgInclude>* includes, bool isAdjacencyInfoRequired)
		: D3D11GpuProgram(source, entryPoint, GPT_DOMAIN_PROGRAM, profile, includes, isAdjacencyInfoRequired)
		, mDomainShader(nullptr)
	{ }

	D3D11GpuDomainProgram::~D3D11GpuDomainProgram()
	{ }

	void D3D11GpuDomainProgram::loadFromMicrocode(D3D11Device& device, ID3D10Blob* microcode)
	{
		HRESULT hr = device.getD3D11Device()->CreateDomainShader(
			static_cast<DWORD*>(microcode->GetBufferPointer()), microcode->GetBufferSize(),
			device.getClassLinkage(), &mDomainShader);

		if (FAILED(hr) || device.hasError())
		{
			String errorDescription = device.getErrorDescription();
			CM_EXCEPT(RenderingAPIException, 
				"Cannot create D3D11 domain shader from microcode.\nError Description:" + errorDescription);
		}
	}

	void D3D11GpuDomainProgram::destroy_internal()
	{
		SAFE_RELEASE(mDomainShader);

		D3D11GpuProgram::destroy_internal();
	}

	ID3D11DomainShader * D3D11GpuDomainProgram::getDomainShader() const
	{
		return mDomainShader;
	}

	D3D11GpuHullProgram::D3D11GpuHullProgram(const String& source, const String& entryPoint,
		GpuProgramProfile profile, const Vector<HGpuProgInclude>* includes, bool isAdjacencyInfoRequired)
		: D3D11GpuProgram(source, entryPoint, GPT_HULL_PROGRAM, profile, includes, isAdjacencyInfoRequired)
		, mHullShader(nullptr)
	{ }

	D3D11GpuHullProgram::~D3D11GpuHullProgram()
	{ }

	void D3D11GpuHullProgram::loadFromMicrocode(D3D11Device& device, ID3D10Blob* microcode)
	{
		// Create the shader
		HRESULT hr = device.getD3D11Device()->CreateHullShader(
			static_cast<DWORD*>(microcode->GetBufferPointer()), microcode->GetBufferSize(),
			device.getClassLinkage(), &mHullShader);

		if (FAILED(hr) || device.hasError())
		{
			String errorDescription = device.getErrorDescription();
			CM_EXCEPT(RenderingAPIException, 
				"Cannot create D3D11 hull shader from microcode.\nError Description:" + errorDescription);
		}
	}

	void D3D11GpuHullProgram::destroy_internal()
	{
		SAFE_RELEASE(mHullShader);

		D3D11GpuProgram::destroy_internal();
	}

	ID3D11HullShader* D3D11GpuHullProgram::getHullShader() const
	{
		return mHullShader;
	}

	D3D11GpuComputeProgram::D3D11GpuComputeProgram(const String& source, const String& entryPoint,
		GpuProgramProfile profile, const Vector<HGpuProgInclude>* includes, bool isAdjacencyInfoRequired)
		: D3D11GpuProgram(source, entryPoint, GPT_COMPUTE_PROGRAM, profile, includes, isAdjacencyInfoRequired)
		, mComputeShader(nullptr)
	{ }

	D3D11GpuComputeProgram::~D3D11GpuComputeProgram()
	{ }

	void D3D11GpuComputeProgram::loadFromMicrocode(D3D11Device& device, ID3D10Blob* microcode)
	{
		HRESULT hr = device.getD3D11Device()->CreateComputeShader(
			static_cast<DWORD*>(microcode->GetBufferPointer()), microcode->GetBufferSize(),
			device.getClassLinkage(), &mComputeShader);

		if (FAILED(hr) || device.hasError())
		{
			String errorDescription = device.getErrorDescription();
			CM_EXCEPT(RenderingAPIException,
				"Cannot create D3D11 compute shader from microcode.\nError Description:" + errorDescription);
		}
	}

	void D3D11GpuComputeProgram::destroy_internal()
	{
		SAFE_RELEASE(mComputeShader);

		D3D11GpuProgram::destroy_internal();
	}

	ID3D11ComputeShader* D3D11GpuComputeProgram::getComputeShader() const
	{
		return mComputeShader;
	}
}