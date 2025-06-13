import {Pipe, PipeTransform} from '@angular/core';

@Pipe({
  name: 'kb'
})
export class KilobytesPipe implements PipeTransform {
  transform(value: number | null | undefined, fractionDigits = 0): string {
    if (value == null || isNaN(value)) return '0 KB';
    const kb = value / 1024;
    return `${kb.toFixed(fractionDigits)} KB`;
  }
}
