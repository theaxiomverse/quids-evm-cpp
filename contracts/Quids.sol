// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import "@openzeppelin/contracts/security/Pausable.sol";
import "@openzeppelin/contracts/access/AccessControl.sol";
import "@openzeppelin/contracts/security/ReentrancyGuard.sol";

/// @title Quids - Quantum-safe Base Currency
/// @notice This is the base currency for the quantum-safe blockchain
contract Quids is ERC20, Pausable, AccessControl, ReentrancyGuard {
    bytes32 public constant MINTER_ROLE = keccak256("MINTER_ROLE");
    bytes32 public constant PAUSER_ROLE = keccak256("PAUSER_ROLE");
    bytes32 public constant BRIDGE_ROLE = keccak256("BRIDGE_ROLE");

    // Quantum signature verification
    mapping(bytes => bool) public qSignatureUsed;
    
    // Cross-chain bridge state
    struct BridgeState {
        uint256 nonce;
        uint256 totalLocked;
        mapping(uint256 => bool) processedNonces;
        mapping(address => uint256) bridgeLimits;
    }
    
    mapping(uint256 => BridgeState) public chainBridges;

    // Events
    event QSignatureVerified(bytes signature, address indexed signer);
    event TokensBridged(uint256 indexed sourceChain, uint256 indexed targetChain, address indexed from, address to, uint256 amount);
    event BridgeLimitUpdated(address indexed account, uint256 newLimit);

    constructor() ERC20("Quids", "QUD") {
        _setupRole(DEFAULT_ADMIN_ROLE, msg.sender);
        _setupRole(MINTER_ROLE, msg.sender);
        _setupRole(PAUSER_ROLE, msg.sender);
        _setupRole(BRIDGE_ROLE, msg.sender);
    }

    /// @notice Mint new tokens with quantum signature
    /// @param to Recipient address
    /// @param amount Amount to mint
    /// @param qSignature Quantum-safe signature
    function qMint(address to, uint256 amount, bytes calldata qSignature) 
        external 
        onlyRole(MINTER_ROLE) 
        whenNotPaused 
    {
        require(!qSignatureUsed[qSignature], "QSignature already used");
        require(verifyQSignature(qSignature), "Invalid quantum signature");
        
        qSignatureUsed[qSignature] = true;
        _mint(to, amount);
        
        emit QSignatureVerified(qSignature, msg.sender);
    }

    /// @notice Bridge tokens to another chain
    /// @param targetChain Target chain ID
    /// @param to Recipient address on target chain
    /// @param amount Amount to bridge
    function bridgeTo(uint256 targetChain, address to, uint256 amount)
        external
        nonReentrant
        whenNotPaused
    {
        require(amount > 0, "Amount must be positive");
        require(amount <= chainBridges[targetChain].bridgeLimits[msg.sender], "Exceeds bridge limit");
        
        BridgeState storage bridge = chainBridges[targetChain];
        bridge.nonce++;
        bridge.totalLocked += amount;
        
        _burn(msg.sender, amount);
        
        emit TokensBridged(block.chainid, targetChain, msg.sender, to, amount);
    }

    /// @notice Receive tokens from another chain
    /// @param sourceChain Source chain ID
    /// @param from Source address
    /// @param to Recipient address
    /// @param amount Amount to receive
    /// @param nonce Transaction nonce
    /// @param qSignature Quantum-safe signature
    function bridgeReceive(
        uint256 sourceChain,
        address from,
        address to,
        uint256 amount,
        uint256 nonce,
        bytes calldata qSignature
    )
        external
        onlyRole(BRIDGE_ROLE)
        nonReentrant
        whenNotPaused
    {
        BridgeState storage bridge = chainBridges[sourceChain];
        require(!bridge.processedNonces[nonce], "Nonce already processed");
        require(!qSignatureUsed[qSignature], "QSignature already used");
        require(verifyQSignature(qSignature), "Invalid quantum signature");
        
        bridge.processedNonces[nonce] = true;
        qSignatureUsed[qSignature] = true;
        
        _mint(to, amount);
        
        emit QSignatureVerified(qSignature, msg.sender);
        emit TokensBridged(sourceChain, block.chainid, from, to, amount);
    }

    /// @notice Set bridge limit for an account
    /// @param account Target account
    /// @param targetChain Target chain ID
    /// @param limit New bridge limit
    function setBridgeLimit(address account, uint256 targetChain, uint256 limit)
        external
        onlyRole(DEFAULT_ADMIN_ROLE)
    {
        chainBridges[targetChain].bridgeLimits[account] = limit;
        emit BridgeLimitUpdated(account, limit);
    }

    /// @notice Verify quantum signature
    /// @param qSignature Quantum-safe signature
    /// @return bool Valid signature
    function verifyQSignature(bytes calldata qSignature) 
        internal 
        pure 
        returns (bool) 
    {
        // Call to external quantum verification contract
        return abi.originalEvm.call(
            address(0x123), // Replace with actual quantum verifier contract
            abi.encodeWithSignature("verifyQSignature(bytes)", qSignature)
        );
    }

    // Pause/unpause functions
    function pause() external onlyRole(PAUSER_ROLE) {
        _pause();
    }

    function unpause() external onlyRole(PAUSER_ROLE) {
        _unpause();
    }

    // Override required functions
    function _beforeTokenTransfer(address from, address to, uint256 amount)
        internal
        whenNotPaused
        override
    {
        super._beforeTokenTransfer(from, to, amount);
    }
} 