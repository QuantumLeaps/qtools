/* SourceId : SPI_SourceId_003 */
/* DesignId : SPI_DesignId_007 */
/* Requirements : HL_SR133 */
uint32 spiReceiveData(spiBASE_t *spi, spiDAT1_t *dataconfig_t,
                         uint32 blocksize, uint16 * destbuff)
{
/* USER CODE BEGIN (6) */
/* USER CODE END */
    uint32 Chip_Select_Hold = (dataconfig_t->CS_HOLD) ? 0x10000000U : 0U;
    uint32 WDelay = (dataconfig_t->WDEL) ? 0x04000000U : 0U;
    SPIDATAFMT_t DataFormat = dataconfig_t->DFSEL;
    uint8 ChipSelect = dataconfig_t->CSNR;

    while(blocksize != 0U)
    {
        if((spi->FLG & 0x000000FFU) !=0U)
        {
          break;
        }
        if(blocksize == 1U)
        {
           Chip_Select_Hold = 0U;
        }

        /*SAFETYMCUSW 51 S MR:12.3 <APPROVED> "Needs shifting for 32-bit value" */
        spi->DAT1 = ((uint32)DataFormat << 24U) |
                    ((uint32)ChipSelect << 16U) |
                    (WDelay)            |
                    (Chip_Select_Hold)  |
                    (0x00000000U);
        /*SAFETYMCUSW 28 D MR:NA <APPROVED> "Hardware status bit read check" */
        while((spi->FLG & 0x00000100U) != 0x00000100U)
        {
        } /* Wait */
        /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are only allowed in this driver" */
        *destbuff = (uint16)spi->BUF;
        /*SAFETYMCUSW 567 S MR:17.1,17.4 <APPROVED> "Pointer increment needed" */
        destbuff++;
        blocksize--;
    }
    return (spi->FLG & 0xFFU);
}
