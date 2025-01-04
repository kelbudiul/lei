#include "type_helper.h"
#include <algorithm>


llvm::Type* TypeHelper::getLLVMType(const Type& type) {
    llvm::Type* baseType = nullptr;

    // Map our type system to LLVM types
    if (type.name == "int") {
        baseType = llvm::Type::getInt32Ty(*context);
    } else if (type.name == "float") {
        baseType = llvm::Type::getDoubleTy(*context);
    } else if (type.name == "bool") {
        baseType = llvm::Type::getInt1Ty(*context);
    } else if (type.name == "void") {
        baseType = llvm::Type::getVoidTy(*context);
    } else if (type.name == "str") {
        baseType = llvm::Type::getInt8PtrTy(*context);
    }

    if (!baseType) {
        // Report unknown type error through the centralized error handler
        errorHandler.error(
            ErrorLevel::CODEGEN,
            0, 0,  // We might want to enhance Location to capture type information
            "Unknown type: " + type.name
        );
        return nullptr;
    }

    // Handle array types
    if (type.isArray && type.arraySize < 0) {
        return baseType->getPointerTo();
    }
    
    // For fixed arrays, we return an array type
    if (type.isArray) {
        return llvm::ArrayType::get(baseType, type.arraySize);
    }

    return baseType;
}

TypeHelper::OperandPair TypeHelper::promoteOperands(llvm::Value* left, llvm::Value* right, const Location& loc) {
    if (!left || !right) {
        errorHandler.error(
            ErrorLevel::CODEGEN,
            loc.line, loc.column,
            "Invalid operands for type promotion"
        );
        return {nullptr, nullptr, nullptr};
    }

    llvm::Type* leftType = left->getType();
    llvm::Type* rightType = right->getType();

    // If either operand is floating point, convert both to floating point
    if (leftType->isDoubleTy() || rightType->isDoubleTy()) {
        llvm::Type* doubleType = llvm::Type::getDoubleTy(*context);
        left = convertToFloatIfNeeded(left);
        right = convertToFloatIfNeeded(right);
        return {left, right, doubleType};
    }

    // If both are integers, use the wider integer type
    if (leftType->isIntegerTy() && rightType->isIntegerTy()) {
        unsigned leftWidth = leftType->getIntegerBitWidth();
        unsigned rightWidth = rightType->getIntegerBitWidth();
        unsigned maxWidth = std::max(leftWidth, rightWidth);

        llvm::Type* commonType = llvm::Type::getIntNTy(*context, maxWidth);
        left = convertToIntN(left, maxWidth);
        right = convertToIntN(right, maxWidth);
        return {left, right, commonType};
    }

    // Return original values if no promotion needed
    return {left, right, leftType};
}

bool TypeHelper::areTypesCompatible(llvm::Type* source, llvm::Type* target) {
    if (!source || !target) return false;
    if (source == target) return true;

    // Handle numeric type compatibility
    if (isNumericType(source) && isNumericType(target)) {
        // Allow integer to float conversion
        if (source->isIntegerTy() && target->isDoubleTy()) return true;
        // Allow same-width integer conversions
        if (source->isIntegerTy() && target->isIntegerTy() &&
            source->getIntegerBitWidth() == target->getIntegerBitWidth()) {
            return true;
        }
    }

    // Handle array/pointer compatibility
    if (isArrayOrPointerType(source) && isArrayOrPointerType(target)) {
        return areTypesCompatible(
            getArrayElementType(source),
            getArrayElementType(target)
        );
    }

    return false;
}

llvm::Value* TypeHelper::convert(llvm::Value* value, llvm::Type* targetType) {
    
    if (!value || !targetType) return nullptr;

    llvm::Type* sourceType = value->getType();
    if (sourceType == targetType) return value;

    // Handle numeric conversions
    if (isNumericType(sourceType) && isNumericType(targetType)) {
        if (sourceType->isIntegerTy() && targetType->isDoubleTy()) {
            return builder->CreateSIToFP(value, targetType, "int.to.float");
        }
        if (sourceType->isDoubleTy() && targetType->isIntegerTy()) {
            return builder->CreateFPToSI(value, targetType, "float.to.int");
        }
        if (sourceType->isIntegerTy() && targetType->isIntegerTy()) {
            return convertToIntN(value, targetType->getIntegerBitWidth());
        }
    }

    // Handle array/pointer conversions
    if (isArrayOrPointerType(sourceType) && isArrayOrPointerType(targetType)) {
        return builder->CreateBitCast(value, targetType, "ptr.cast");
    }

    return nullptr;
}

bool TypeHelper::isNumericType(llvm::Type* type) const {
    return type && (type->isIntegerTy() || type->isDoubleTy());
}

bool TypeHelper::isArrayOrPointerType(llvm::Type* type) const {
    return type && (type->isArrayTy() || type->isPointerTy());
}

llvm::Type* TypeHelper::getArrayElementType(llvm::Type* type) {
    if (!type) return nullptr;
    if (type->isArrayTy()) return type->getArrayElementType();
    if (type->isPointerTy()) return type->getPointerElementType();
    return nullptr;
}

llvm::Value* TypeHelper::convertToFloatIfNeeded(llvm::Value* value) {
    if (!value->getType()->isDoubleTy()) {
        return builder->CreateSIToFP(value, 
            llvm::Type::getDoubleTy(*context), "to.float");
    }
    return value;
}

llvm::Value* TypeHelper::convertToIntN(llvm::Value* value, unsigned width) {
    llvm::Type* targetType = llvm::Type::getIntNTy(*context, width);
    unsigned sourceWidth = value->getType()->getIntegerBitWidth();
    
    if (sourceWidth == width) return value;
    if (sourceWidth < width) {
        return builder->CreateSExt(value, targetType, "int.extend");
    }
    return builder->CreateTrunc(value, targetType, "int.trunc");
}

llvm::Value* TypeHelper::getArrayElementPtr(llvm::Value* array, llvm::Value* index) {
    if (!array || !index) return nullptr;

    // First, check if we're dealing with a pointer type
    llvm::Type* ptrType = array->getType();
    if (!ptrType->isPointerTy()) {
        errorHandler.error(
            ErrorLevel::CODEGEN,
            0, 0,
            "Array base must be a pointer type"
        );
        return nullptr;
    }

    // Get the element type that the pointer points to
    llvm::Type* elementType = ptrType->getPointerElementType();
    
    // Handle both fixed arrays and dynamic arrays
    if (auto arrayType = llvm::dyn_cast<llvm::ArrayType>(elementType)) {
        // For fixed-size arrays, we need two indices: array base and element
        std::vector<llvm::Value*> indices = {
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
            index
        };
        return builder->CreateInBoundsGEP(elementType, array, indices, "array.elem");
    } else {
        // For dynamic arrays (pointers), we use a single index
        return builder->CreateInBoundsGEP(elementType, array, index, "array.elem");
    }
}