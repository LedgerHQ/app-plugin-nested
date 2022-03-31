import Zemu, { DEFAULT_START_OPTIONS, DeviceModel } from '@zondax/zemu';
import Eth from '@ledgerhq/hw-app-eth';
import { generate_plugin_config } from './generate_plugin_config';
import { parseEther, parseUnits, RLP } from "ethers/lib/utils";
import { ethers } from "ethers";
import ledgerService from "@ledgerhq/hw-app-eth/lib/services/ledger"

const transactionUploadDelay = 60000;

async function waitForAppScreen(sim) {
    await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot(), transactionUploadDelay);
}

const sim_options_nano = {
    ...DEFAULT_START_OPTIONS,
    logging: true,
    X11: true,
    startDelay: 5000,
    startText: 'is ready'
};

const Resolve = require('path').resolve;

const NANOS_ETH_PATH = Resolve('elfs/ethereum_nanos.elf');
const NANOX_ETH_PATH = Resolve('elfs/ethereum_nanox.elf');

const NANOS_PLUGIN_PATH = Resolve('elfs/plugin_nanos.elf');
const NANOX_PLUGIN_PATH = Resolve('elfs/plugin_nanox.elf');

const nano_models: DeviceModel[] = [
    { name: 'nanos', letter: 'S', path: NANOS_PLUGIN_PATH, eth_path: NANOS_ETH_PATH },
    { name: 'nanox', letter: 'X', path: NANOX_PLUGIN_PATH, eth_path: NANOX_ETH_PATH }
];


const nestedJSON = generate_plugin_config();

const SPECULOS_ADDRESS = '0xFE984369CE3919AA7BB4F431082D027B4F8ED70C';
const RANDOM_ADDRESS = '0xaaaabbbbccccddddeeeeffffgggghhhhiiiijjjj'

const resolutionConfig = {
    externalPlugins: true,
    nft: false,
    erc20: false
};

const loadConfig = {
    nftExplorerBaseURL: "https://nft.api.live.ledger.com/v1/ethereum",
    // nftExplorerBaseURL: null,
    pluginBaseURL: "https://cdn.live.ledger.com",
    // pluginBaseURL: null,
    extraPlugins: nestedJSON,
}

let genericTx = {
    nonce: Number(0),
    gasLimit: Number(21000),
    gasPrice: parseUnits('1', 'gwei'),
    value: parseEther('1'),
    chainId: 137,
    to: RANDOM_ADDRESS,
    data: null,
};

const TIMEOUT = 1000000;

// Generates a serializedTransaction from a rawHexTransaction copy pasted from etherscan.
function txFromEtherscan(rawTx) {
    // Remove 0x prefix
    rawTx = rawTx.slice(2);

    let txType = rawTx.slice(0, 2);
    if (txType == "02" || txType == "01") {
        // Remove "02" prefix
        rawTx = rawTx.slice(2);
    } else {
        txType = "";
    }

    let decoded = RLP.decode("0x" + rawTx);
    if (txType != "") {
        decoded = decoded.slice(0, decoded.length - 3); // remove v, r, s
    } else {
        decoded[decoded.length - 1] = "0x"; // empty
        decoded[decoded.length - 2] = "0x"; // empty
        decoded[decoded.length - 3] = "0x01"; // chainID 1
    }

    // Encode back the data, drop the '0x' prefix
    let encoded = RLP.encode(decoded).slice(2);

    // Don't forget to prepend the txtype
    return txType + encoded;
}

async function resolveTxFromData(data, contractAddr) {
    let unsignedTx = genericTx;

    unsignedTx.to = contractAddr;
    unsignedTx.data = data;
    unsignedTx.value = parseEther("1");
    console.log(`PENZOICI chainid: ${unsignedTx.chainId}`)

    const serializedTx = ethers.utils.serializeTransaction(unsignedTx).slice(2);

    const resolution = await ledgerService.resolveTransaction(
        serializedTx,
        loadConfig,
        resolutionConfig
    );

    return [resolution, serializedTx];
}

function signTransaction(serializedTx, resolution, signTransaction) {
    const tx = signTransaction(
        "44'/60'/0'/0",
        serializedTx,
        resolution
    );
    return tx;
}

function zemu(device, func) {
    return async () => {
        jest.setTimeout(TIMEOUT);
        let elf_path;
        let lib_elf;
        elf_path = device.eth_path;
        lib_elf = { 'Nested': device.path };

        const sim = new Zemu(elf_path, lib_elf);
        try {
            await sim.start({ ...sim_options_nano, model: device.name });
            const transport = sim.getTransport();
            // const transport = await sim.getTransport();
            const eth = new Eth(transport);
            eth.setLoadConfig({
                baseURL: null,
                extraPlugins: nestedJSON,
            });
            await func(sim, eth);
        } finally {
            await sim.close();
        }
    };
}

module.exports = {
    zemu,
    waitForAppScreen,
    genericTx,
    nano_models,
    SPECULOS_ADDRESS,
    RANDOM_ADDRESS,
    txFromEtherscan,
    resolveTxFromData,
    signTransaction,
    resolutionConfig,
    loadConfig,
}
